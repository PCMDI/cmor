import hashlib

import os

ls=os.popen("ls Tables/CMIP5*")

if os.path.exists("Tables/md5s"):
    f=open("Tables/md5s")
    tbls=eval(f.read())
    f.close()
else:
    tbls={}

for l in ls:
    fnm = l.strip()
    f=open(fnm)
    t=f.read()
    i=t.find("table_id:")
    id=t[i+9:].split("\n")[0].split()[-1]
    i=t.find("table_date:")
    date = t[i+11:].split("\n")[0].split("!")[0].strip()
    i=t.find("project_id:")
    pid = t[i+11:].split("\n")[0].split("!")[0].strip()
    md5=hashlib.md5(t)
    md5=md5.hexdigest()
    print fnm,pid,id,date,md5
    pdic = tbls.get(pid,{})
    tdic = pdic.get(id,{})
    if tdic.has_key(date):
        print "WARNING: Replacing exisiting date for date %s in table %s of project %s" % (date,id,pid)
    tdic[date]=md5
    pdic[id]=tdic
    tbls[pid]=pdic

f=open("Tables/md5s","w")
f.write(repr(tbls))
f.close()
