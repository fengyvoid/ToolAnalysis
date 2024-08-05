import sys

run = sys.argv[1]
pi = sys.argv[2]
pf = sys.argv[3]

file = open('ANNIEEventTreeMakerConfig', "w")

file.write('ANNIEEventTreeMakerVerbosity 0\n')
file.write('\n')
file.write('fillAllTriggers 1\n')
file.write('fill_singleTrigger 0\n')
file.write('fillLAPPDEventsOnly 0\n')
file.write('TankCluster_fill 1\n')
file.write('cluster_TankHitInfo_fill 1\n'
file.write('\n')
file.write('OutputFile BeamCluster_' + str(run) + '_' + str(pi) + '_' + str(pf) + '.ntuple.root\n')
file.write('TankClusterProcessing 1\n')
file.write('MRDClusterProcessing 1\n')
file.write('TriggerProcessing 1\n')
file.write('TankHitInfo_fill 1\n')
file.write('MRDHitInfo_fill 1\n')
file.write('MRDReco_fill 1\n')
file.write('SiPMPulseInfo_fill 0\n')
file.write('fillCleanEventsOnly 0\n')
file.write('MCTruth_fill 0\n')
file.write('Reco_fill 1\n')
file.write('TankReco_fill 0\n')
file.write('RecoDebug_fill 0\n')
file.write('muonTruthRecoDiff_fill 0\n')
file.write('IsData 1\n')
file.write('HasGenie 0')
file.write('\n')
file.write('LAPPDData_fill 1\n')
file.write('LAPPDReco_fill 1\n')
file.write('RWMBRF_fill 1\n')
file.write('LAPPD_PPS_fill 1\n')

file.close()
