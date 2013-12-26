import sys,os,time,xml.dom.minidom,zlib,struct

def check_attributes(element,name_list):
	for name in name_list:
		if not element.attributes.has_key(name):
			raise "element %s without attribute %s" % (element.tagName,name)

class Shader:
	def __init__(self,group,xml_element):
		check_attributes(xml_element,["name","source"])
		self.name=xml_element.getAttribute("name")
		self.source=xml_element.getAttribute("source")
		if xml_element.attributes.has_key("defines"):
			self.defines=xml_element.getAttribute("defines")
		else:
			self.defines=None
		if xml_element.attributes.has_key("profile"):
			self.profile=xml_element.getAttribute("profile")
		else:
			self.profile=group.profile
		self.group=group

	def build_asm(self):
		cmdfxc=self.group.library.fxc
		cmdbase="/T%s /Emain /Fcasm/%s.asm" % (self.profile,self.name)
		if self.defines:
			cmdopt=" /D%s" % (self.defines)
		else:
			cmdopt=""
		cmdline="%s %s%s src/%s" % (cmdfxc,cmdbase,cmdopt,self.source)
		print ".",
		f=os.popen(cmdline,"r")
		output=[line for line in f]
		res=f.close()
		if res:
			print " %s" % (cmdline)
			raise "Shader compilation failed!"

	def build_bin(self):
		cmdpsa=self.group.library.psa
		cmdbase="/Fobin/%s.bin asm/%s.asm" % (self.name,self.name)
		cmdline="%s %s" % (cmdpsa,cmdbase)
		print ".",
		f=os.popen(cmdline,"r")
		output=[line for line in f]
		res=f.close()
		if res:
			print " %s" % cmdline
			raise "Shader assembly failed!"
	
	def compress(self):
		f=file("bin/%s.bin" % self.name,"rb")
		sh_data=f.read()
		f.close()
		self.uncompressed_size=len(sh_data)
		self.compressed=zlib.compress(sh_data,9)
		
	def write_to_header(self,f_out):
		f_out.write("\tnamespace %s\n\t{\n" % self.name)
		f_out.write("\t\tconst unsigned int index=%d;\n" % self.index)
		if self.profile[:2]=="ps":
			f_out.write("\t\ttypedef PDIRECT3DPIXELSHADER9 sh_type;\n\n")
		else:
			f_out.write("\t\ttypedef PDIRECT3DVERTEXSHADER9 sh_type;\n\n")
		#open .asm file and parse it (CLUMSY CODE WARNING)
		f_asm=file("asm/%s.asm" % self.name,"r")
		line=f_asm.readline()
		reading_params=False
		reading_registers=False
		typemap={}
		clist=[]
		slist=[]
		while line[:2]=="//":
			ls=line.split()
			if reading_params:
				if len(ls)==2 and ls[1]=="Registers:":
					reading_params=False
					reading_registers=True
					#skip 3 lines
					line=f_asm.readline()
					line=f_asm.readline()
					line=f_asm.readline()
				else:
					if len(ls)==3:
						typemap[ls[2][:-1]]=ls[1]
			else:
				if reading_registers:
					if len(ls)==4:
						if ls[2][:1]=="c":
							clist.append((ls[1],ls[2],ls[3]))
						else:
							slist.append((ls[1],ls[2],ls[3]))
				else:
					if len(ls)==2 and ls[1]=="Parameters:":
						reading_params=True
			line=f_asm.readline()
		f_asm.close()
		#write out sampler and constant info
		if len(clist)>0:
			f_out.write("\t\tenum constant_ids\n\t\t{\n")
			for (name,reg,size) in clist:
				extra_tabs=6-(len(name)+len(reg[1:])+2)/4
				adj_str=""
				if extra_tabs<0:
					extra_tabs=0
				for i in range(0,extra_tabs):
					adj_str="%s\t" % adj_str					
				f_out.write("\t\t\tcid_%s=%s,%s//%s\n" % (name,reg[1:],adj_str,typemap[name]))
			f_out.write("\t\t}\n\n")
		if len(slist)>0:
			f_out.write("\t\tenum sampler_ids\n\t\t{\n")
			for (name,reg,size) in slist:
				extra_tabs=6-(len(name)+len(reg[1:])+2)/4
				adj_str=""
				if extra_tabs<0:
					extra_tabs=0
				for i in range(0,extra_tabs):
					adj_str="%s\t" % adj_str					
				f_out.write("\t\t\tsid_%s=%s,%s//%s\n" % (name,reg[1:],adj_str,typemap[name]))
			f_out.write("\t\t}\n\n")
		f_out.write("\t}\n\n")
		
class Group:
	def __init__(self,library,xml_element):
		check_attributes(xml_element,["name","profile"])
		self.name=xml_element.getAttribute("name")
		self.profile=xml_element.getAttribute("profile")
		self.shaders=[Shader(self,shader_node) for shader_node in xml_element.getElementsByTagName("shader")]
		self.library=library
		
	def build_asm(self):
		for shader in self.shaders:
			shader.build_asm()

	def build_bin(self):
		for shader in self.shaders:
			shader.build_bin()
			
	def compress(self):
		for shader in self.shaders:
			shader.compress()
			
	def write_header(self):
		f_out=file("include/%s_%s.h" % (self.library.name,self.name),"w")
		f_out.write("//Automatically generated header file\n")
		f_out.write("//Do not edit manually!!!\n\n")
		f_out.write("namespace %s\n{\n" % self.library.name)
		for shader in self.shaders:
			shader.write_to_header(f_out)
		f_out.write("}\n")
		f_out.close()

class Library:
	def __init__(self,xml_element):
		check_attributes(xml_element,["name","fxc","psa","vsa"])
		self.name=xml_element.getAttribute("name")
		self.fxc=xml_element.getAttribute("fxc")
		self.psa=xml_element.getAttribute("psa")
		self.vsa=xml_element.getAttribute("vsa")
		self.groups=[Group(self,group_node) for group_node in xml_element.getElementsByTagName("group")]
		
	def build(self):
		print "Building shader library %s" % (self.name)
		print "Compiling HLSL:"
		compile_start=time.clock()
		for group in self.groups:
			group.build_asm()
		compile_len=(time.clock()-compile_start)*1000
		print "\nAssembling binary shader files:"
		asm_start=time.clock()
		for group in self.groups:
			group.build_bin()
		asm_len=(time.clock()-asm_start)*1000
		lib_start=time.clock()
		offsets=[]
		current_offset=0
		uncompressed_size=0
		total_shader_count=0
		sh_index=0
		for group in self.groups:
			group.compress()
			total_shader_count+=len(group.shaders)
			for shader in group.shaders:
				offsets.append(current_offset)
				current_offset+=len(shader.compressed)
				uncompressed_size+=shader.uncompressed_size
				shader.index=sh_index
				sh_index+=1
		#file format:
		#int - shader count N
		#(int,int,int){N} - tuple (offset,uncompressed size,compressed size)
		#shader data, zlib compressed, concatenated
		fixup=total_shader_count*12+4
		offsets=[offset+fixup for offset in offsets]
		f_out=file("lib/%s.lib" % self.name,"wb")
		f_out.write(struct.pack("<i",total_shader_count))
		for group in self.groups:
			for shader in group.shaders:
				f_out.write(struct.pack("<i",offsets[shader.index]))
				f_out.write(struct.pack("<i",shader.uncompressed_size))
				f_out.write(struct.pack("<i",len(shader.compressed)))
		for group in self.groups:
			for shader in group.shaders:
				f_out.write(shader.compressed)
		f_out.close()
		#include with enums
		f_out=file("include/%s_base_enums.h" % self.name,"w")
		f_out.write("//Automatically generated header file\n")
		f_out.write("//Do not edit manually!!!\n\n")
		f_out.write("namespace %s\n{\n" % self.name)
		f_out.write("\tstatic const char szLibraryName[]=\"%s\";\n" % self.name)
		f_out.write("}\n")
		f_out.close()
		#include with all shaders
		f_out=file("include/%s_all.h" % self.name,"w")
		f_out.write("//Automatically generated header file\n")
		f_out.write("//Do not edit manually!!!\n\n")
		f_out.write("#include \"%s_base_enums.h\"\n" % self.name)
		for group in self.groups:
			f_out.write("#include \"%s_%s.h\"\n" % (self.name,group.name))
		f_out.close()
		for group in self.groups:
			group.write_header()
		lib_len=(time.clock()-lib_start)*1000
		print "\n\nStatistics:"
		print "Shader count: %d" % total_shader_count
		print "Compilation time: %d ms" % compile_len
		print "Assembly time: %d ms" % asm_len
		print "Library build time: %d ms" % lib_len

if len(sys.argv)<2:
	raise "Please give xml build spec as first parameter"
dom=xml.dom.minidom.parse(sys.argv[1])
if dom.documentElement.tagName!="library":
	raise "Root element must use \"library\" tag name"
lib=Library(dom.documentElement)
lib.build()
