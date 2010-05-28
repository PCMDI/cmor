import numpy,sys,genutil


def afunc(func,vls,nms,prepend='',nxtrm=2,verbose=True):        
    vals=(vls,)
    out=func(*vals)
    if verbose: print '\t\t%s%s: %g' % (prepend,func.__name__,out)
    args = numpy.ma.argsort(vls)
    if verbose:
        print '\t\t\tExtremes: ',
        for i in range(nxtrm):
            if len(nms)>i:
                print '%s, %g; ' % (nms[args[i]],vls[args[i]]),
        for i in range(nxtrm):
            if len(nms)>i:
                print '%s, %g; ' % (nms[args[len(nms)-i-1]],vls[args[len(nms)-i-1]]),
        print
    vls = numpy.ma.sort(vls)
    done = False
    while nxtrm>0 and done is False:
        if len(nms)>nxtrm*2:
            vls=vls[nxtrm:-nxtrm]
            vals=(vls,)
            out = func(*vals)
            done = True
            if verbose: print '\t\t\t%s%s (no extremes): %g' % (prepend,func.__name__,out)
        else:
            nxtrm-=1
    return out


def process_file(file,doavg=False,results={},verbose=True):
    vars={}
    f=open(file)
    offset=0
    if doavg:
        offset=1

    for l in f.xreadlines():
        sp=l.split()
        if len(sp)==1:
            continue
        var=sp[1]
        vr=vars.get(var,{})
        modl = sp[0].split("/")[10]
        if sp[2]=='N/A':
            lev='N/A'
        else:
            lev=str(int(float(sp[2])*100)/100)
        levd=vr.get(lev,{})
        mins = levd.get("mins",[])
        maxs = levd.get("maxs",[])
        avgs = levd.get("avgs",[])
        amins = levd.get("amins",[])
        amaxs = levd.get("amaxs",[])
        aavgs = levd.get("aavgs",[])
        modnm = levd.get("names",[])
        try:
            mn = float(sp[3])
        except:
            mn = 1.e20
        try:
            mx = float(sp[4])
        except:
            mx = 1.e20
        if doavg:
            try:
                avg = float(sp[5])
            except:
                avg = 1.e20
        else:
            avg=1.e20
        try:
            amn = float(sp[5+offset])
        except:
            amn = 1.e20
        try:
            amx = float(sp[6+offset])
        except:
            amx = 1.e20
        if doavg:
            try:
                aavg = float(sp[8])
            except:
                aavg = 1.e20
        else:
            aavg=1.e20

        if modl in modnm:
            mmn = mins.pop(modnm.index(modl))
            mmx = maxs.pop(modnm.index(modl))
            mavg = avgs.pop(modnm.index(modl))
            mamn = amins.pop(modnm.index(modl))
            mamx = amaxs.pop(modnm.index(modl))
            maavg = aavgs.pop(modnm.index(modl))
            mnm = modnm.pop(modnm.index(modl))
            if mn!=1.e20 :  mn = min(mmn,mn)
            if mx!=1.e20 :  mx = max(mmx,mx)
            if avg!=1.e20 :  avg = (avg+mavg)/2.
            if amn!=1.e20 : amn = min(mamn,amn)
            if amx!=1.e20 : amx = max(mamx,amx)
            if aavg!=1.e20 :  aavg = (aavg+maavg)/2.

        mins.append(mn)
        maxs.append(mx)
        avgs.append(avg)
        amins.append(amn)
        amaxs.append(amx)
        aavgs.append(aavg)
        modnm.append(modl)

        levd['mins']=mins
        levd['maxs']=maxs
        levd['avgs']=avgs
        levd['amins']=amins
        levd['amaxs']=amaxs
        levd['names']=modnm
        levd['aavgs']=aavgs
        vr[lev]=levd
        vars[var]=vr

    vrs=vars.keys()
    vrs.sort()
    for vr in vrs:
        if verbose: print vr
        if vr in results.keys():
            print 'Var:',vr,'already exists skipping'
            continue
        V={}
        var=vars[vr]
        #print 'vaR:',var
        levs = var.keys()
        levs.sort()
        for l in levs:
            L=var[l]
            mins=numpy.ma.masked_greater(L['mins'],1.e20)
            maxs=numpy.ma.masked_greater(L['maxs'],1.e20)
            avgs=numpy.ma.masked_greater(L['avgs'],1.e20)
            amins=numpy.ma.masked_greater(L['amins'],1.e20)
            amaxs=numpy.ma.masked_greater(L['amaxs'],1.e20)
            aavgs=numpy.ma.masked_greater(L['aavgs'],1.e20)
            nms = L['names']
            if verbose: print '\tLevel:',l,len(nms),'models',len(mins),len(avgs)
            
            Mn  = afunc(numpy.ma.min,mins,nms,verbose=verbose)
            Mna = afunc(numpy.ma.average,mins,nms,'Mins ',verbose=verbose)
            Mns = afunc(genutil.statistics.std,mins,nms,'Mins ',verbose=verbose)

            Mx  = afunc(numpy.ma.max,maxs,nms,verbose=verbose)
            Mxa = afunc(numpy.ma.average,maxs,nms,'Maxs ',verbose=verbose)
            Mxs = afunc(genutil.statistics.std,maxs,nms,'Maxs ',verbose=verbose)

            if doavg:
                Am = afunc(numpy.ma.min,avgs,nms,'Averages ',verbose=verbose)
                AM = afunc(numpy.ma.max,avgs,nms,'Averages ',verbose=verbose)
                Aa = afunc(numpy.ma.average,avgs,nms,'Averages ',verbose=verbose)
                As = afunc(genutil.statistics.std,avgs,nms,'Averages ',verbose=verbose)
                
            AMn  = afunc(numpy.ma.min,amins,nms,'Absolute ',verbose=verbose)
            AMna = afunc(numpy.ma.average,amins,nms,'Abs. Mins ',verbose=verbose)
            AMns = afunc(genutil.statistics.std,amins,nms,'Abs. Mins ',verbose=verbose)
            
            AMx  = afunc(numpy.ma.max,amaxs,nms,'Absolute ',verbose=verbose)
            AMxa = afunc(numpy.ma.average,amaxs,nms,'Abs. Maxs ',verbose=verbose)
            AMxs = afunc(genutil.statistics.std,amaxs,nms,'Abs. Maxs ',verbose=verbose)
            
            if doavg:
                AAm = afunc(numpy.ma.min,aavgs,nms,'Abs. Averages ',verbose=verbose)
                AAM = afunc(numpy.ma.max,aavgs,nms,'Abs. Averages ',verbose=verbose)
                AAa = afunc(numpy.ma.average,aavgs,nms,'Abs. Averages ',verbose=verbose)
                AAs = afunc(genutil.statistics.std,aavgs,nms,'Abs. Averages ',verbose=verbose)

            Lev={'Min':{'min':Mn,'avg':Mna,'std':Mns},
                 'Max':{'max':Mx,'avg':Mxa,'std':Mxs},
                 'AMin':{'min':AMn,'avg':AMna,'std':AMns},
                 'AMax':{'max':AMx,'avg':AMxa,'std':AMxs}
                 }
            if doavg:
                Lev['Avg']={'max':AM,'min':Am,'avg':Aa,'std':As}
                Lev['AAvg']={'max':AAM,'min':AAm,'avg':AAa,'std':AAs}
            V[l]=Lev
        results[vr]=V
    f.close()
    return results


if __name__=="__main__":
    import pickle

    f=open("Tables_csv/minmax.pickled",'w')

    myvars={}
    for fnm in sys.argv[1:]:
        print 'Processing:',fnm
        myvars = process_file(fnm,doavg=True,results=myvars,verbose=False)
        print len(myvars)

    pickle.dump(myvars,f)
    f.close()
    #print myvars
