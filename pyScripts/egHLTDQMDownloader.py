import os, sys, urllib2, httplib, json
from ROOT import *
from array import *

serverurl = 'https://cmsweb.cern.ch/dqm/offline'
ident = "DQMToJson/1.0 python/%d.%d.%d" % sys.version_info[:3]
HTTPS = httplib.HTTPSConnection

class X509CertAuth(HTTPS):
    ssl_key_file = None
    ssl_cert_file = None

    def __init__(self, host, *args, **kwargs):
        HTTPS.__init__(self, host,
                       key_file = X509CertAuth.ssl_key_file,
                       cert_file = X509CertAuth.ssl_cert_file,
                       **kwargs)

class X509CertOpen(urllib2.AbstractHTTPHandler):
    def default_open(self, req):
        return self.do_open(X509CertAuth, req)

def x509_params():
    key_file = cert_file = None

    x509_path = os.getenv("X509_USER_PROXY", None)
    if x509_path and os.path.exists(x509_path):
        key_file = cert_file = x509_path

    if not key_file:
        x509_path = os.getenv("X509_USER_KEY", None)
    if x509_path and os.path.exists(x509_path):
        key_file = x509_path

    if not cert_file:
        x509_path = os.getenv("X509_USER_CERT", None)
    if x509_path and os.path.exists(x509_path):
        cert_file = x509_path

    if not key_file:
        x509_path = os.getenv("HOME") + "/.globus/userkey.pem"
    if os.path.exists(x509_path):
        key_file = x509_path

    if not cert_file:
        x509_path = os.getenv("HOME") + "/.globus/usercert.pem"
    if os.path.exists(x509_path):
        cert_file = x509_path

    if not key_file or not os.path.exists(key_file):
        print >>sys.stderr, "no certificate private key file found"
        sys.exit(1)

    if not cert_file or not os.path.exists(cert_file):
        print >>sys.stderr, "no certificate public key file found"
        sys.exit(1)

   # print "Using SSL private key", key_file
   # print "Using SSL public key", cert_file
    return key_file, cert_file

def dqm_get_json(server, run, dataset, folder):
    X509CertAuth.ssl_key_file, X509CertAuth.ssl_cert_file = x509_params()
    datareq = urllib2.Request('%s/data/json/archive/%s/%s/%s?rootcontent=1'
                              % (server, run, dataset, folder))
    datareq.add_header('User-agent', ident)
    # return json.load(urllib2.build_opener(X509CertOpen()).open(datareq))
    return eval(urllib2.build_opener(X509CertOpen()).open(datareq).read(),
                { "__builtins__": None }, {})


def get_hists(serverurl, run, dataset, base_folder, path_folder,out_file):
    folder = base_folder+"/"+path_folder
    data = dqm_get_json(serverurl, run, dataset, folder)
   # print data
    for item in data['contents']:
        if 'obj' in item.keys() and 'rootobj' in item.keys(): 
            #skip HEP17/HEM17 
            if item['obj'].find("HEP17")!=-1 or item['obj'].find("HEM17")!=-1: continue
          #  print item
            a = array('B')
            a.fromstring(item['rootobj'].decode('hex'))
            buffer_file = TBufferFile(TBufferFile.kRead, len(a), a, kFALSE)
            rootType = item['properties']['type']
           # print rootType
            if rootType == 'TH1F' or rootType == 'TH2F' :
                hist = buffer_file.ReadObject(eval(rootType+'.Class()'))
                hist.SetDirectory(out_file)

def get_hists_for_dataset_runnr(dataset,run,out_file):
    folder = '/HLT/EGM/TagAndProbeEffs'
    dataset_foroutdir = dataset.lstrip("/").replace("/","--")
    base_outdir = "DQMData/"+dataset_foroutdir+"/Run "+run+"/HLT/Run summary/EGTagAndProbeEffs"
    out_file.mkdir(base_outdir)
    
    paths_whitelist = ["HLT_Ele32_WPTight_Gsf","HLT_DoubleEle25_CaloIdL_MW","HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL","HLT_Ele115_CaloIdVT_GsfTrkIdT"]

    contents=[]
    
    data = dqm_get_json(serverurl, run, dataset, folder)
    contents = data['contents']
            
    for item in contents:
        #print item
        if 'subdir' in item.keys():
            if item['subdir'] in paths_whitelist:
                out_dir =base_outdir+"/"+item['subdir']
                out_file.mkdir(out_dir).cd()
                get_hists(serverurl, run, dataset, folder, item['subdir'], out_file.GetDirectory(out_dir))
"""
returns a dictionary of datasets with the runs in that dataset as the payload"
"""
def get_datasets_runs(dataset_pattern):
    
    datasets_runs = {}
    query = "dataset dataset="+dataset_pattern
    datasets,err = subprocess.Popen(["dasgoclient","--query",query],stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
    for dataset in datasets.split("\n"):
        if dataset == '': continue
        query = "run dataset="+dataset
        runs,err = subprocess.Popen(["dasgoclient","--query",query],stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
        if dataset not in datasets_runs:
            datasets_runs[dataset]=[]
        for run in runs.split("\n"):
            if run == '': continue
            datasets_runs[dataset].append(run)

    return datasets_runs
    
"""
returns a dictionary of datasets with the runs in that dataset as the payload
"""
def get_datasets_runs_in_file(file_):
    datasets_runs = {}
    old_dir = gDirectory
    file_.cd("DQMData")

    for key in gDirectory.GetListOfKeys():
        print key.GetName(),key.GetClassName()
        if key.GetClassName().find("TDirectory")==0:
            dataset_name = "/"+key.GetName().replace("--","/")
            if dataset_name not in datasets_runs:
                datasets_runs[dataset_name] = []
                for subdir in gDirectory.Get(key.GetName()).GetListOfKeys():
                    print subdir.GetName()
                    datasets_runs[dataset_name].append(subdir.GetName().split()[1])

    old_dir.cd()
    return datasets_runs

if __name__ == "__main__":

    import os
    import argparse
    import subprocess
    
    parser = argparse.ArgumentParser(description='downloads the DQM histograms for the E/g HLT DQM')
    
    parser.add_argument('--runs',nargs="+",help='runs or file containing runs',default=[])
    parser.add_argument('--output',help='output filename',required=True)
    parser.add_argument('--dataset',help='dataset',default='/EGamma/Run2018A-PromptReco-v{}/DQMIO')  
    parser.add_argument('--update',action='store_true',help='updates an existing file, skipping runs already in it')

    args = parser.parse_args()

    dataset = args.dataset
    if args.update:
        out_file = TFile(args.output,"UPDATE")
        datasets_runs_in_file = get_datasets_runs_in_file(out_file)
    else:
        out_file = TFile(args.output,"RECREATE")
        datasets_runs_in_file = {}

    datasets_runs = get_datasets_runs(args.dataset)

    
    #if runs is specificed read it from file if it is a file otherwise its a list of runs
    if args.runs:
        try:
            with open(args.runs[0]) as f:
                runs=f.readlines()
        except IOError:
            runs = args.runs
    #if not specificed, run over all runs
    else:
        runs = []
        for dataset in datasets_runs:
            runs.extend(datasets_runs[dataset])

    for run in runs:
        run = run.rstrip()
        for dataset in datasets_runs:
            if run in datasets_runs[dataset] and \
            (dataset not in datasets_runs_in_file or run not in datasets_runs_in_file[dataset]):
                print "processing run",run,"dataset",dataset
                trynr=0
                while trynr<3:
                    try: 
                        get_hists_for_dataset_runnr(dataset,run,out_file)
                        trynr=3
                    except urllib2.URLError:
                        print "URL error re-trying ",trynr+1
                        trynr+=1
    out_file.Write()
    out_file.Close()
