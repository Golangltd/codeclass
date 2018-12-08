Set shell = Wscript.createobject("wscript.shell")
currentpath = createobject("Scripting.FileSystemObject").GetFolder(".").Path
dim fso,file,read,string_arr(),count,string_finally
set fso=createobject("scripting.filesystemobject")
file="c:\YLG_config.ini" '文件
fso.createtextfile(file).write currentpath  '写入file文件
