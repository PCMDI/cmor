import xlrd

bk=xlrd.open_workbook("Tables_csv/standard_output.xls")#,encoding_override="utf32")

for i in range(bk.nsheets):
    sh = bk._sheet_list[i]
    print 'Treating sheet:',sh.name
    f=open("Tables_csv/"+str(sh.name).split(" ")[0]+'.csv','w')
    for r in range(sh.nrows):
        line=''
        try:
            for c in range(sh.ncols):
                cell=sh.cell(r,c).value
                try:
                    cell=str(cell)
                except:
                    cell=cell.replace(u"\ufb01","fi")
                    cell=cell.replace(u"\u2013","-")
                    cell=cell.replace(u"\u2026","...")
                    cell=cell.replace(u"\xb0","") #degree o
                    cell=cell.replace(u"\u201c",'"')
                    cell=cell.replace(u"\u201d",'"')
                    cell=str(cell)
                cell=cell.replace('"','""')
                if cell.find(",")>-1 or cell.find('"')>-1:
                    line+='"%s",' % cell
                else:
                    line+='%s,' % cell
        except Exception,err:
            print 'Error row: %i, col: %i, value: %s' % (r,c,sh.cell(r,c))
            line+=','
        line=line[:-1]
        print >> f,line
    print >> f,"\n"
    f.close()
            
