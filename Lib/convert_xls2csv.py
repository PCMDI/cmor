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
                    #ok this bit is for Karl showing the problem
                    for (bad,rpl) in  [ (u"\ufb01","fi"),
                                        (u"\u2013","-"),
                                        (u"\u2026","..."),
                                        (u"\xb0",""),
                                        (u"\u201c",'"'),
                                        (u"\u201d",'"'),
                                        ]:
                        while cell.find(bad)>-1:
                            print 'Bad character row %i, col %s, value: %s' % (r+1,chr(c+65),sh.cell(r,c))
                            cell=cell.replace(bad,rpl,1)
                    cell=str(cell)
                cell=cell.replace('"','""')
                if cell.find(",")>-1 or cell.find('"')>-1:
                    line+='"%s",' % cell
                else:
                    line+='%s,' % cell
        except Exception,err:
            print 'Error row: %i, col: %i, value: %s' % (r,c,sh.cell(r,c))
            print err
            line+=','
        line=line[:-1]
        print >> f,line
    print >> f,"\n"
    f.close()
            
