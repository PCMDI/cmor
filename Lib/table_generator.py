import sys,time,os,genutil


prefix = "CMIP5"

general = """cmor_version: 2.0         ! version of CMOR that can read this table
cf_version:   1.4         ! version of CF that output conforms to
project_id:   %s  ! project id
table_date:   %s ! date this table was constructed

missing_value: 1.e20      ! value used to indicate a missing value
                          !   in arrays output by netCDF as 32-bit IEEE 
                          !   floating-point numbers (float or real)

baseURL: http://cmip-pcmdi.llnl.gov/CMIP5/dataLocation 
product: output

required_global_attributes: creation_date tracking_id forcing model_id parent_experiment_id branch_time contact institute_id ! space separated required global attribute 

forcings:   N/A Nat Ant GHG SD SI SA TO SO Oz LU Sl Vl SS Ds BC MD OC AA

expt_id_ok: '10- or 30-year run initialized in year XXXX' 'decadalXXXX'
expt_id_ok: 'volcano-free hindcasts XXXX' 'noVolcXXXX'
expt_id_ok: 'prediction with 2010 volcano' 'volcIn2010'
expt_id_ok: 'pre-industrial control' 'piControl'
expt_id_ok: 'Historical' 'historical'
expt_id_ok: 'mid-Holocene' 'midHolocene'
expt_id_ok: 'last glacial maximum' 'lgm'
expt_id_ok: 'last millennium' 'past1000'
expt_id_ok: 'RCP4.5' 'rcp45'
expt_id_ok: 'RCP8.5' 'rcp85'
expt_id_ok: 'RCP2.6' 'rcp26'
expt_id_ok: 'RCP6' 'rcp60'
expt_id_ok: 'ESM pre-industrial control' 'esmControl'
expt_id_ok: 'ESM historical' 'esmHistorical'
expt_id_ok: 'ESM RCP8.5' 'esmrcp85'
expt_id_ok: 'ESM fixed climate 1' 'esmFixClim1'
expt_id_ok: 'ESM fixed climate 2' 'esmFixClim2'
expt_id_ok: 'ESM feedback 1' 'esmFdbk1'
expt_id_ok: 'ESM feedback 2' 'esmFdbk2'
expt_id_ok: '1 percent per year CO2' '1pctCO2'
expt_id_ok: 'abrupt 4XCO2' 'abrupt4xCO2'
expt_id_ok: 'natural-only' 'historicalNat'
expt_id_ok: 'GHG-only' 'historicalGHG'
expt_id_ok: 'anthropogenic-only' 'historicalAnt'
expt_id_ok: 'anthropogenic sulfate aerosol direct effect only' 'historicalSD'
expt_id_ok: 'anthropogenic sulfate aerosol indirect effect only' 'historicalSI'
expt_id_ok: 'anthropogenic sulfate aerosol only' 'historicalSA'
expt_id_ok: 'tropospheric ozone only' 'historicalTO'
expt_id_ok: 'stratospheric ozone' 'historicalSO'
expt_id_ok: 'ozone only' 'historicalOz'
expt_id_ok: 'land-use change only' 'historicalLU'
expt_id_ok: 'solar irradiance only' 'historicalSl'
expt_id_ok: 'volcanic aerosol only' 'historicalVl'
expt_id_ok: 'sea salt only' 'historicalSS'
expt_id_ok: 'dust' 'historicalDs'
expt_id_ok: 'black carbon only' 'historicalBC'
expt_id_ok: 'mineral dust only' 'historicalMD'
expt_id_ok: 'organic carbon only' 'historicalOC'
expt_id_ok: 'anthropogenic aerosols only' 'historicalAA'
expt_id_ok: 'AMIP' 'amip'
expt_id_ok: '2030 time-slice' 'sst2030'
expt_id_ok: 'control SST climatology' 'sstClim'
expt_id_ok: 'CO2 forcing' 'sstClim4xCO2'
expt_id_ok: 'all aerosol forcing' 'sstClimAerosol'
expt_id_ok: 'sulfate aerosol forcing' 'sstClimSulfate'
expt_id_ok: '4xCO2 AMIP' 'amip4xCO2'
expt_id_ok: 'AMIP plus patterned anomaly' 'amipFuture'
expt_id_ok: 'aqua planet control' 'aquaControl'
expt_id_ok: '4xCO2 aqua planet' 'aqua4xCO2'
expt_id_ok: 'aqua planet plus 4K anomaly' 'aqua4K'
expt_id_ok: 'AMIP plus 4K anomaly' 'amip4K'

""" % (prefix,time.strftime("%d %B %Y"))



#realm:	      %s
#table_id:     Table %s    ! table id


axis_tmpl  = """
!============
axis_entry: %(CMOR dimension)
!============
!----------------------------------
! Axis attributes:
!----------------------------------
standard_name:    %(standard name)
units:            %(units)
axis:             %(axis)             ! X, Y, Z, T (default: undeclared)
positive:         %(positive)         ! up or down (default: undeclared)
long_name:        %(long name)
!----------------------------------
! Additional axis information:
!----------------------------------
out_name:         %(output dimension name)
valid_min:        %(valid_min)         
valid_max:        %(valid_max) 
stored_direction: %(stored direction)
formula:          %(formula)
z_factors:        %(z_factors)
z_bounds_factors: %(z_factor_bounds)
tolerance:        %(tol_on_requests: variance from requested values that is tolerated)
type:             %(type)
requested:        %(requested)        ! space-separated list of requested coordinates 
requested_bounds: %(bounds_ requested) ! space-separated list of requested coordinate bounds
value:            %(value)            ! of scalar (singleton) dimension 
bounds_values:    %(bounds _values)    ! of scalar (singleton) dimension bounds
must_have_bounds: %(bounds?)
index_only:       %(index axis?)
climatology:      %(climatology)
coords_attrib:    %(coords_attrib)
!----------------------------------
!
"""


var_tmpl = """!============
variable_entry:    %(CMOR variable name)
!============
modeling_realm:    %(realm)
!----------------------------------
! Variable attributes:
!----------------------------------
standard_name:     %(standard name)
units:             %(unformatted units)
cell_methods:      %(cell_methods)
cell_measures:      %(cell_measures)
long_name:         %(long name)
comment:           %(comment)
!----------------------------------
! Additional variable information:
!----------------------------------
dimensions:        %(CMOR dimensions)
out_name:          %(output variable name)
type:              %(type)
positive:          %(positive)
valid_min:         %(valid min)
valid_max:         %(valid max)
ok_min_mean_abs:   %(mean absolute min)
ok_max_mean_abs:   %(mean absolute max)
flag_values:       %(flag values)
flag_meanings:     %(flag meaning)
!----------------------------------
!
"""

def process_a_line(line):
    #print line
    if line.find("usiiiiually,")>-1:
        debug = True
        print
        print 'NEW STUFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
        print
    else:
        debug=False
    line=line.replace("\r\n","\n") # dos character 
    line = line.replace('""','"$$$')
    sps = line .split(",")
    sp=[]
    st=""
    while (len(sps)>0):
        s = sps.pop(0)
        if debug:
            print 's:',s
        if len(s)>0 and s[0]=='"' and s[-1]!='"':
            if debug: print 'inthere'
            s=s[1:]
            s2=sps.pop(0)
            if debug: print 's2:',s2
            while len(s2)==0 or s2[-1]!='"':
                s = ','.join([s,s2])
                #print sps
                s2=sps.pop(0)
            else:
                s = ','.join([s,s2[:-1]])                
        sp.append(s.replace("$$$",'"'))
    return sp


def process_template(tmpl,cnames,cols,voids={},minmax={}):
    F = genutil.StringConstructor(tmpl)

    keys = F.keys()
    match = 0
    for c in cnames:
        if c in keys:
            match+=1
            indx = cnames.index(c)
##             print ' matched at %i' % indx ,
            if indx<len(cols):
                val = cols[indx]
            else:
                val = ""
            if  val.strip()=='time2':
                setattr(F,"climatology","yes")
                if "climatology" in keys: keys.remove("climatology")
            if val.strip()!="":
                if c in ['units','unformatted units'] and val=='1.0':
                    val='1'
                setattr(F,c,val)
                keys.remove(c)
##                 print
##             else:
##                 print ' but empty'
##         else:
##             print
    #print 'Keys:',keys
    if "CMOR dimension" in keys:
        print 'Keys:',keys
        print 'cnames:',cnames
        raise "crap"
    ve = getattr(F,"CMOR variable name","yep not that guy") 
    if ve in minmax.keys():
        if 'valid min' in keys:
        #ok let's see if we can figure this one out
            mnmx = minmax[ve]
            val=1.e20
            std=0.
            for mlev in mnmx.keys():
                mn=mnmx[mlev]['Min']
                val = min(mn['min'],val)
                std +=mn['std']
            std/=len(mnmx.keys())
            setattr(F,"valid min","%.2g" % (val-2*std))
            keys.remove("valid min")
        if 'valid max' in keys:
        #ok let's see if we can figure this one out
            mnmx = minmax[ve]
            val=-1.e20
            std=0.
            for mlev in mnmx.keys():
                mn=mnmx[mlev]['Max']
                val = max(mn['max'],val)
                std +=mn['std']
            std/=len(mnmx.keys())
            setattr(F,"valid max","%.2g" % (val+2*std))
            keys.remove("valid max")

        ### Need to add lines for absolute mean min/max
                
    for k in keys:
        setattr(F,k,"!CRAP WE NEED TO REMOVE THAT LINE")

    ## Now generates
    out = F()
    sp = out.split("\n")
    lines = []
    for l in sp:
        if l.find("!CRAP WE NEED TO REMOVE THAT LINE")>-1:
            continue
        lines.append(l)
    out = "\n".join(lines)
            
##     print 'We got: %i matches' % match
    # fixes Karl input bug
    out = out.replace("..",".")

    #Ok now check the void thing
    for kw in voids.keys():
        v = getattr(F,kw,"we keep").strip()
        vals = voids[kw]
        if not isinstance(vals,(list,tuple)):
            vals = [vals,]
        for V in vals:
            if V == v:
                out = ""
    return out

def create_table_header(tbnm, table_file, dims_file, fqcy):
    #First of All create the header
    fnm = "Tables/" + prefix + '_'+tbnm
    fo = open(fnm,'w')
    print >> fo, "table_id: Table %s" % tbnm

    realm = None
    if tbnm[0]=='O':
        realm = "ocean"
    elif tbnm[0]=='A':
        realm = 'atmos'
    elif tbnm[0]=='L':
        realm = 'land'
    else:
        realm = "atmos"


    print >> fo, "modeling_realm: %s\n" % realm
    print >> fo, "frequency: %s\n" % fqcy
    print >> fo, general

    # looking for approx interval, ASSUMING UNITS ARE IN DAYS SINCE
    if tbnm.find("mon")>-1:
        interval = 30.
    elif tbnm.lower().find('clim')>-1:
        interval = 30.
    elif tbnm.lower().find('aero')>-1:
        interval = 30.
    elif tbnm.lower().find('yr')>-1:
        interval = 365.
    elif tbnm.lower().find('da')>-1:
        interval = 1.
    elif tbnm.find("hr")==1:
        interval = float(tbnm[0])/24.
    elif tbnm.find("min")>-1:
        interval = float(tbnm[2:tbnm.find("min")])/1440.
    else:
        interval = 0.
    print >> fo, """approx_interval:  %f     ! approximate spacing between successive time
                          !   samples (in units of the output time 
                          !   coordinate.""" % interval

    D = open(dims_file)
    dlines = D.readlines()[2:]
    i=0
    while dlines[i].strip()=="":
        i+=1
    dlines=dlines[i:]
    cnms = dlines[0].split(',')
    for i in  range(len(cnms)):
        cnms[i]=cnms[i].strip()


    addLines = False
    for l in dlines[1:]:
        sp = process_a_line(l)
        foundnm = False
        for snm in sp[0].split(","):
            if tbnm == snm.strip():
                foundnm  = True
        if foundnm:
            if l.find("alevel")>-1:
                addLines = True
                zlevel_name = 'alevel'
                file_add = 'Tables_csv/add_dims.txt'
            elif l.find("olevel")>-1:
                addLines = True
                zlevel_name = 'olevel'
                file_add = 'Tables_csv/add_dims2.txt'
                if tbnm=='fx':
                    file_add= "Tables_csv/add_dims2_notime.txt"
            else:
                print >> fo, process_template(axis_tmpl,cnms,sp)
    if addLines is True:
        print 'adding:',file_add,'to',tbnm
        tmpf=open(file_add)
        lns=tmpf.readlines()
        tmpf.close()
        if file_add == 'Tables_csv/add_dims.txt':
            if not tbnm in ['aero','cfMon']:
                lns=lns[:-20]
        lns=''.join(lns)
        lns=lns.replace("zlevel",zlevel_name)
        print >> fo, lns

    return fo


def create_table(table_file, dims_file,minmax={}):
    tables = {}
    foundnm= False
    D = open(table_file)
    dlines = D.readlines()
    ivar = -2
    if len(dlines)==1:
        dlines = dlines[0].split('\r')
    #This bit insert a table
    dlines2=[]
    for i in range(len(dlines)):
        if dlines[i].find("include Amon 2D")>-1:
            f=open("Tables_csv/Amon_2D.csv")
            add_lines = f.readlines()
            if table_file[-11:-4] in ['cfSites','v/cf3hr']:
                tmplines=[]
                for aline in add_lines:
                    a_line =aline.strip()
                    if len(a_line)==0:
                        continue
                    if a_line[-1]!=',' : a_line=a_line+','
                    sp = process_a_line(a_line)
                    ## for i2 in range(len(sp)):
                    ##     if sp[i2]=='tasmax' : print i2,sp[i2]
                    if len(sp)>15 and 'time' in sp[16]:
                        sp[16]=sp[16].replace('time','time1')
                        if table_file[-11:-4]=='cfsites':
                            sp[16]=sp[16].replace("longitude latitude","site")
                        ## print 'Replaced to:',sp[16]
                    for i in range(len(sp)):
                        if sp[i].find(",")>-1:
                            sp[i]='"%s"' % sp[i]
                    myline = ','.join(sp)
                    myline = myline.replace("time: mean","time: point")
                    if len(sp)>4 and sp[5] in ['tasmin','tasmax']:
                        print 'Skipping tasmin/max'
                        pass
                    else:
                        tmplines.append(myline)
                add_lines = tmplines
            dlines2=dlines2+add_lines
        elif dlines[i].find("include Oyr")>-1:
            f=open("Tables_csv/Oyr_tracer.csv")
            dlines2=dlines2+f.readlines()
        else:
            dlines2.append(dlines[i])
    dlines=dlines2
    for l in dlines:
        #print 'processing:',l.strip()
        sp = process_a_line(l)
        if 0<=sp[0].find("CMOR Table")<=1 and foundnm == False: # line that will give us the table name
            i=1
            while sp[i].strip()=="":
                i+=1
            tbnm = sp[i].strip()
            fqcy = sp[i+1].strip()
            foundnm = True
        if sp[0] == 'priority':
            cnms = sp
            for i in  range(len(cnms)):
                cnms[i]=cnms[i].strip()
            ivar = cnms.index("CMOR variable name")
            continue
        if ivar!=-2 and len(sp)>ivar and sp[ivar].strip()!="":
            if tbnm in tables.keys():
                fo = tables[tbnm]
            else: # New table
                print 'Creating table:',tbnm
                fo = create_table_header(tbnm,table_file,dims_file,fqcy)
                tables[tbnm]=fo
            print >> fo, process_template(var_tmpl,cnms,sp,{'CMOR variable name':['?','0','0.0']},minmax=minmax)
    print 'Created tables:',tables.keys()
                
        

    

if __name__== "__main__" :
    import extract_min_max
    if len(sys.argv)>2:
        dims_table = sys.argv[2]
    else:
        dims_table = 'Tables_csv/dims.csv'

    if len(sys.argv)>1:
        print sys.argv
        create_table(sys.argv[1],dims_table)
    else:
        tables_nms = """
Tables_csv/cfOff.csv    Tables_csv/Omon.csv     Tables_csv/Amon.csv
Tables_csv/cfMon.csv    Tables_csv/Oclim.csv    Tables_csv/6hrPlev.csv
Tables_csv/fx.csv       Tables_csv/cfDay.csv    Tables_csv/OImon.csv    
Tables_csv/cf3hr.csv    Tables_csv/Lmon.csv     Tables_csv/3hr.csv
Tables_csv/day.csv      Tables_csv/aero.csv     Tables_csv/LImon.csv
Tables_csv/cfSites.csv  Tables_csv/Oyr.csv      Tables_csv/6hrLev.csv
""".split()

##         tables_nms = """Tables_csv/3hr.csv      Tables_csv/amon.csv     Tables_csv/cfMon.csv    Tables_csv/oclim.csv
## Tables_csv/6hrLev.csv   Tables_csv/cfsites.csv  Tables_csv/cfOff.csv    Tables_csv/fx.csv       Tables_csv/olmon.csv
## Tables_csv/6hrPlev.csv  Tables_csv/cf3hr.csv    Tables_csv/llmon.csv    Tables_csv/omon.csv
## Tables_csv/aero.csv     Tables_csv/cfDa.csv     Tables_csv/da.csv       Tables_csv/lmon.csv     Tables_csv/oyr.csv
## """.split()
        import pickle
        f=open("Tables_csv/minmax.pickled")
        minmax = pickle.load(f)
        for nm in tables_nms:
            print 'Processing:',nm
            create_table(nm,dims_table,minmax=minmax)
