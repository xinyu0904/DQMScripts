# DQMScripts
cmsrel CMSSW_9_4_4
cd CMSSW_9_4_4/src
cmsenv
git cms-init
git clone https://github.com/cms-egamma/DQMScripts DQMScripts
scram b -j 9
python pyScripts/egHLTDQMDownloader.py --output egammaDQMHists.root --dataset /EGamma/Run2018A-PromptReco-v\*/DQMIO
