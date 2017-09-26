#!/usr/bin/env python

import os
import argparse
import ROOT   
import glob
import shutil
import sys

def find_nth(string, substring, n):
    start = string.find(substring)
    while start >= 0 and n > 1:
        start = string.find(substring, start+len(substring))
        n -= 1
    return start

class RelValInfo:
    cmssw_version_full = ""
    cmssw_version = ""
    pu_type = ""
    sample_type = ""
    sample_type_full = ""
    global_tag = ""
    
    def __init__(self,filename):
        filename_split = filename.split("__")
        gt_pu_start = find_nth(filename_split[2],"-",1)
        gt_end = filename_split[2].rfind("-v")
        self.cmssw_version_full = filename_split[2][:gt_pu_start]
        self.cmssw_version = self.cmssw_version_full[6:].replace('_','')

        #now figure out if there is PU which starts at the front of this string if so
        #otherwise its just the GT
        gt_pu_str = filename_split[2][gt_pu_start+1:gt_end]        
        if gt_pu_str[:2]=="PU":
            self.pu_type = gt_pu_str.split('_')[0]
            self.global_tag = gt_pu_str.split('_')[1]
        else:
            self.pu_type = ""
            self.global_tag = gt_pu_str

        self.sample_type_full = filename_split[1]
        self.sample_type = self.sample_type_full[6:-3]
    
#        print self.cmssw_version_full,self.cmssw_version,self.pu_type,self.global_tag,self.sample_type_full,self.sample_type


def makeRelValPlots(filename,ref_filename,base_output_dir,update):
    


    sample_info = RelValInfo(filename)
    ref_info = RelValInfo(ref_filename)
    
    subdir_name="EGRelVal_"+sample_info.sample_type+"_"+sample_info.cmssw_version+sample_info.pu_type+"Vs"+ref_info.cmssw_version+ref_info.pu_type

    output_dir = base_output_dir.rstrip("/")+"/"+subdir_name
    
    leg_entry = sample_info.cmssw_version
    if sample_info.pu_type != "": leg_entry+="-"+sample_info.pu_type
    refleg_entry = ref_info.cmssw_version+ref_info.pu_type
    if ref_info.pu_type != "": leg_entry+="-"+ref_info.pu_type

    if os.path.isdir(output_dir):
        if update:
            shutil.rmtree(output_dir)
        else:
            print "\n",output_dir,"exists already, use --update option to delete it and remake it\n"
            sys.exit(0)

    ROOT.gROOT.ProcessLine(".L rootScripts/makeDQMHLTRelValPlots.C+")
    ROOT.printAllPlots(output_dir,filename,ref_filename,leg_entry,refleg_entry)
    
    subdirs = glob.glob(output_dir+"/*")
    for subdir in subdirs:
        files = glob.glob(subdir+"/*.gif")
        with open(subdir+"/index.html","w") as f:
            for filename in files:
                filename = filename.split("/")[-1]
                if subdir.split("/")[-1] == "EGTagAndProbe":
                    html_str = "Path: {} Filter: {} <br>\n".format(filename.split('-')[1],filename.split('-')[2].split(".")[0])
                    f.write(html_str)
                html_str = "<a href=\"{}\"><img class=\"image\" width=\"1000\" src=\"{}\" ALIGH=TOP></a><br><br>\n".format(filename,filename)
                f.write(html_str)
    with open(output_dir+"/index.html","w") as f:
        for subdir in subdirs:
            html_str = "<a href=\"{}\">{}</a><br>\n".format(subdir.split("/")[-1],subdir.split("/")[-1])
            f.write(html_str)
    
       
                

    


if __name__ == "__main__":    
    parser = argparse.ArgumentParser(description='reads DQM histogram files and produces formated plots for easier validation')
    parser.add_argument('filename',help='filename')
    parser.add_argument('refFilename',help='reference filename')
    parser.add_argument('-o','--outputDir',help='output base directory',required=True)
    parser.add_argument('--update',action='store_true',help='allows overwriting of existing directory')
    args = parser.parse_args()

    makeRelValPlots(args.filename,args.refFilename,args.outputDir,args.update)
