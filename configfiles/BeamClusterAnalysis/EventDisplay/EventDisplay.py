import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.patches as patches
import pandas as pd
from mpl_toolkits.axes_grid1.inset_locator import inset_axes
from matplotlib.colors import ListedColormap
from matplotlib.ticker import MaxNLocator
import uproot3 as uproot
import glob
import decimal
from decimal import Decimal, getcontext
import numpy as np
import matplotlib.pyplot as plt
from tqdm import tqdm
import datetime
import pytz
import os


########### Path YOU NEED to set
########### An example was also attached

# Path of MRD geometry file
# Use the MRDGeometry from LoadGeometry, but remove the titles. An example was attached in this folder
MRDFile = '/Users/fengy/ANNIESofts/MyForkCode/ToolAnalysis/configfiles/LoadGeometry/FullMRDGeometry_09_29_20.csv'

# Path of the folder with ANNIE BeamCluster root tree
path = '/Users/fengy/ANNIESofts/Analysis/9.10-FinalVersion/trees/'

# file pattern to be loaded
file_pattern = path + 'LAPPDBeamCluster_4763.root'

# Save path and name of the pdf
SavePath =  '/Users/fengy/ANNIESofts/Analysis/9.10-FinalVersion/scripts/'
pdfName = 'TestNewEventDisplay.pdf'

# maximum number of events to be displayed
printEventMaxNumber = 100
printPDF = True


######################## BEGIN the CoDe

# cuts for the event display
cut_beamOK = True
cut_clusterExist = True
cut_clusterCB = True
cut_clusterCB_value = 0.2
cut_clusterMaxPE = False
cut_clusterMaxPE_value = 500

cut_MRDactivity = False
cut_noVeto = False #require event doesn't have veto activity
cut_hasTrack = True
cut_MRDPMTCoinc = True

# If True, Require the PMTs around the LAPPD to have PE > 5
cut_ChrenkovCover = False # in an LAPPD event, for the brightest PMT cluster, require at least n of the nearing PMTs have pe value
cut_ChrenkovCover_nPMT = 4 #
cut_ChrenkovCover_PMT_PE = 5 # 5pe on each PMT
PMT_chanKey = [[462,428,406,412]] # for 2022 and 2023 data
#PMT_chanKey = [[374,377,407,445],[463,411,400,404],[462,428,406,412]] # for 2024 data

cut_LAPPDMultip = True
cut_LAPPDHitAmp = 5
cut_LAPPDHitNum = 5


plt.rcParams["font.family"] = "Times New Roman"



#################################################################################
################### Load geometry and defind LAPPD geometry here   ##############
################### all in cm
#################################################################################
# tank center position in X, Y, Z
TC = [0,-14.4649,168.1]
TRadius = 152
THeight = 396
TPMTRadius = 100
TPMTHeight = 145*2

# LAPPD position
LAPPD_Center = [[0,-0.1265,295.1]]
LAPPD_stripWidth = 0.462
LAPPD_stripSpace = 0.229
LAPPD_H_width = 20
LAPPD_V_height = 20

# dead PMT position
deadPMTs = [
    [-0.2476846, -0.1882051, 0.14804416, 0.34590855, -0.3860695, 0.47238569, -0.1523917, -1.0413511, 0.26391394, -0.5454545, 0.54989219],  #X
    [-1.6939623, -1.6939623, -1.6939623, -1.6986855, -1.6939623, -1.6939623, 1.32865366, 0.86344388, -1.2980369, -1.2864868, -0.8586892],  #Y
    [1.6037927, 0.91420765, 1.20706832, 1.00870613,1.11256984, 1.52414908,2.4965238, 1.40794291, 2.72564427, 2.60815843, 2.6087467],  #Z
    [333, 342, 343, 345, 346, 349, 352, 416, 431, 444, 445]   #channel key
]
deadPMTs = [[i - TC[0] for i in deadPMTs[0]], [i - TC[1]/100 for i in deadPMTs[1]], [i - TC[2]/100 for i in deadPMTs[2]], deadPMTs[3]]

# MRD position
with open(MRDFile, 'r') as file:
    lines = file.readlines()
df = pd.read_csv(MRDFile, skiprows=0)
MRDGeo = df.set_index('channel_num').T.to_dict()
class MRDGeoAccessor:
    def __init__(self, data):
        self.data = data
    
    def __getattr__(self, attr):
        def get_by_channel(channel_num):
            return self.data[channel_num][attr]
        return get_by_channel
MRDGeoAccessor = MRDGeoAccessor(MRDGeo)

# filter the pmt hits for display
def process_pmt_hits(PMTHits):
    hitX, hitY, hitZ, hitT, hitPE, hitChanKey = PMTHits
    hitT = np.array(hitT)
    hitChanKey = np.array(hitChanKey)
    startHitT = np.min(hitT)
    time_mask = hitT < startHitT + 50
    unique_keys, key_indices = np.unique(hitChanKey, return_index=True)
    PMTHits_merged = [[] for _ in range(5)]
    for key_index in key_indices:
        channel_mask = hitChanKey == hitChanKey[key_index]
        combined_mask = channel_mask & time_mask
        if np.any(combined_mask):
            min_t_index = np.argmin(hitT[combined_mask])
            hit_index = np.where(combined_mask)[0][min_t_index]
            PMTHits_merged[0].append(hitX[hit_index])
            PMTHits_merged[1].append(hitY[hit_index])
            PMTHits_merged[2].append(hitZ[hit_index])
            PMTHits_merged[3].append(hitT[hit_index])
            PMTHits_merged[4].append(hitPE[hit_index])
    
    return PMTHits_merged




#################################################################################
################### Event Display Function   ####################################
#################################################################################


# event display 

def EventDisplay(pdf, EventInfo, PMTClusterInfo, PMTHits_raw, ExtendedCluster, MRDHits, MRDTracks, LAPPDHits, hasVeto):
    # print this event into a pdf
    
    '''
    input information:
    select and pass the right information to this function 
    PMTHits = [hitX, hitY, hitZ, hitT, hitPE, hitChanKey]
    MRDHits = [MRDhitChankey, MRDhitT]
    MRDTracks = [MRDTrackStartX, MRDTrackStopX, MRDTrackStartY, MRDTrackStopY, MRDTrackStartZ, MRDTrackStopZ]
    LAPPDHits = [LAPPDID, LAPPDHitStrip, LAPPDHitTime, LAPPDHitAmp]
    '''
    PMTHits_raw_4_array = np.array(PMTHits_raw[4])
    valid_indices = np.isfinite(PMTHits_raw_4_array)
    for ind in range (len(PMTHits_raw)):
        HitArray = np.array(PMTHits_raw[ind])
        filtered_PMTHits = HitArray[valid_indices]
        PMTHits_raw[ind] = filtered_PMTHits.tolist()


    
    PMTHits = process_pmt_hits(PMTHits_raw)
    
    fig, axs = plt.subplots(3, 3, figsize=(15, 16))
    for ax in fig.get_axes():
        ax.remove()
    
    ax_tank = plt.subplot2grid((3, 3), (0, 0), colspan=2, rowspan=2)
    ax_tank.axis('off')
    
    axmuyz = plt.subplot2grid((3, 3), (2, 0)) #muon y-z
    axmuyz.axis('off')
    axmuxz = plt.subplot2grid((3, 3), (2, 1)) #muon x-z
    axmuxz.axis('off')
    ax_l0 = plt.subplot2grid((3, 3), (0, 2)) #LAPPD 0 hit time
    ax_l1 = plt.subplot2grid((3, 3), (1, 2)) #LAPPD 1 hit time
    ax_l2 = plt.subplot2grid((3, 3), (2, 2)) #LAPPD 2 hit time
    
    
    inset_ax =  plt.subplot2grid((15, 5), (1, 2),colspan = 1, rowspan = 3)
    cmap = plt.cm.viridis  # 获取 'viridis' 颜色映射
    new_cmap = ListedColormap(cmap(np.linspace(0, 1, cmap.N)))  # 创建新的映射
    new_cmap.set_under('white')  # 设置低于vmin部分为白色
    h = inset_ax.hist2d(PMTHits_raw[3], PMTHits_raw[4], bins=[30, 30], cmap=new_cmap, vmin=0.1)
    pos = inset_ax.get_position()

    inset_cbar = plt.colorbar(h[3], ax=inset_ax)
    inset_cbar.set_label('Counts', fontsize=10)
    inset_ax.set_ylabel('PMT Hit PE', fontsize=10)
    inset_ax.set_xlabel('Hit Time (ns)', fontsize=10)
    inset_ax.set_title('PMT Hit PE vs Time', fontsize=12)
    
    if(len(ExtendedCluster[0])>0):
        ext_ax =  plt.subplot2grid((21, 5), (11, 2),colspan = 1, rowspan = 2)
        ext = ext_ax.scatter(ExtendedCluster[0], ExtendedCluster[1], s = 20)
        ext_ax.set_ylabel('Extended Cluster PE', fontsize=10)
        ext_ax.set_xlabel('Extended Cluster Time (ns)', fontsize=10)
        ext_ax.set_title('Extended Cluster', fontsize=12)
        ext_ax.xaxis.set_major_locator(MaxNLocator(integer=True, nbins=5))
        ext_ax.yaxis.set_major_locator(MaxNLocator(integer=True, nbins=4))
        
        
        dPE = max(ExtendedCluster[1]) - min(ExtendedCluster[1])
        dT = max(ExtendedCluster[0]) - min(ExtendedCluster[0])
        if(dPE != 0):
            ext_ax.set_ylim(min(ExtendedCluster[1]) - 0.1*dPE, max(ExtendedCluster[1]) + 0.1*dPE)
        else:
            ext_ax.set_ylim(min(ExtendedCluster[1]) - 5, min(ExtendedCluster[1]) + 5) 
        ext_ax.set_xlim(2000, max(ExtendedCluster[0]) + 0.1*dT)

    
    ####################################################
    #########        plot tank PMTs            #########
    ####################################################
    Hit_2DX = []
    Hit_2DY = []
    Hit_PE = []
    
    maxPE = max(PMTHits[4])
    secondMaxPE = max([x for x in PMTHits[4] if x != maxPE])
    normMax = maxPE
    if(maxPE > 2*secondMaxPE):
        normMax = secondMaxPE
    
    for h in range(len(PMTHits[0])):
        if(PMTHits[4][h]<2):
            continue
            
        plotThis = True
        if(PMTHits[4][h]<0.2*normMax):
            plotThis = False
            ##check if there is a hit within 0.6m has PE > 10%
            for n in range (len(PMTHits[0])):
                if(PMTHits[4][n]>0.2*normMax):
                    distance = np.sqrt((PMTHits[0][n] - PMTHits[0][h])**2 + (PMTHits[1][n] - PMTHits[1][h])**2 + (PMTHits[2][n] - PMTHits[2][h])**2 )
                    if(distance<0.6):
                        plotThis = True
                        break
                        
        if(plotThis != True):
            continue
            
        Hit_PE.append(PMTHits[4][h])
        if(PMTHits[1][h]> 1.3):    # top
            Hit_2DX.append(PMTHits[0][h])
            Hit_2DY.append(TC[1]/100 + (THeight/2 + TRadius)/100 - (PMTHits[2][h]))
        elif(PMTHits[1][h]< -1.4):  # bottom
            Hit_2DX.append(PMTHits[0][h])
            Hit_2DY.append(TC[1]/100 - (THeight/2 + TRadius)/100 + (PMTHits[2][h]))
        else:
            Hit_2DY.append(PMTHits[1][h])
            dX = PMTHits[0][h]
            dZ = PMTHits[2][h]
            phi = abs(np.arctan(dZ/dX)/np.pi)
            distance_X = (phi+0.5)*np.pi*(TRadius/100)
            if (dX>0 and dZ>0):
                distance_X = ((0.5-phi)*np.pi*(TRadius/100))
            elif (dX<0 and dZ>0):
                distance_X = -((0.5-phi)*np.pi*(TRadius/100))
            elif (dX<0 and dZ<0):
                distance_X = -((0.5+phi)*np.pi*(TRadius/100))
            Hit_2DX.append(distance_X)
            
    Hit_2DX = [hit*100 for hit in Hit_2DX]
    Hit_2DY = [hit*100 for hit in Hit_2DY]
    
    # dead PMTs
    Hit_2DX_dead = []
    Hit_2DY_dead = []
    
    for h in range(len(deadPMTs[0])):
        if(deadPMTs[1][h]> 1.3):    # top
            Hit_2DX_dead.append(deadPMTs[0][h])
            Hit_2DY_dead.append(TC[1]/100 + (THeight/2 + TRadius)/100 - (deadPMTs[2][h]))
        elif(deadPMTs[1][h]< -1.4):  # bottom
            Hit_2DX_dead.append(deadPMTs[0][h])
            Hit_2DY_dead.append(TC[1]/100 - (THeight/2 + TRadius)/100 + (deadPMTs[2][h]))
        else:
            Hit_2DY_dead.append(deadPMTs[1][h])
            dX = deadPMTs[0][h]
            dZ = deadPMTs[2][h]
            phi = abs(np.arctan(dZ/dX)/np.pi)
            distance_X = (phi+0.5)*np.pi*(TRadius/100)
            if (dX>0 and dZ>0):
                distance_X = ((0.5-phi)*np.pi*(TRadius/100))
            elif (dX<0 and dZ>0):
                distance_X = -((0.5-phi)*np.pi*(TRadius/100))
            elif (dX<0 and dZ<0):
                distance_X = -((0.5+phi)*np.pi*(TRadius/100))
            Hit_2DX_dead.append(distance_X)
            
    
    Hit_2DX_dead = [hit*100 for hit in Hit_2DX_dead]
    Hit_2DY_dead = [hit*100 for hit in Hit_2DY_dead]

    
    dead = ax_tank.scatter(Hit_2DX_dead, Hit_2DY_dead, color = 'grey', alpha = 0.5, s = 200)

        
    #sc = ax_tank.scatter(Hit_2DX, Hit_2DY, c=Hit_PE, cmap='inferno', norm=plt.Normalize(vmin=0, vmax=normMax), s = 200)
    sc = ax_tank.scatter(Hit_2DX, Hit_2DY, c=Hit_PE, cmap='viridis', norm=plt.Normalize(vmin=0, vmax=normMax), s = 200)
    #cbar = plt.colorbar(sc, ax=ax_tank)
    #cbar.set_label('PE', fontsize = 20)
    #cbar.ax.tick_params(labelsize=15)
    #cbar.ax.set_position([0.85, 0.2, 0.2, 0.2])

    #divider = make_axes_locatable(ax_tank)
    #cax = fig.add_axes([ax.get_position().x1+0.01,ax.get_position().y0,0.02,ax.get_position().height])
    #cax = divider.append_axes("right", size="3%", pad=0)  # width 1%, pad 0.1
    #pos = cax.get_position()  # 获取当前 color bar 的位置
    #cax.set_position([pos.x0, pos.y0 + pos.height * 0.35, pos.width, pos.height * 0.3])  # x0不变，高度30%，居中
    # 创建 colorbar
    cbar = plt.colorbar(sc, ax=ax_tank, shrink = 0.3, anchor=(0.0, 0.3), pad = 0, fraction = 0.1)
    cbar.set_label('PMT Hit PE', fontsize=15)
    cbar.ax.tick_params(labelsize=15)


    rect = plt.Rectangle((-np.pi*TRadius, TC[1] - THeight/2), 2*np.pi*TRadius, THeight, linewidth=2, edgecolor='black', facecolor='none')
    ax_tank.add_patch(rect)
    circle1 = plt.Circle((TC[0], TC[1] + THeight/2 + TRadius), TRadius, linewidth=2, edgecolor='black', facecolor='none')
    ax_tank.add_patch(circle1)
    circle2 = plt.Circle((TC[0], TC[1] - THeight/2 - TRadius), TRadius, linewidth=2, edgecolor='black', facecolor='none')
    ax_tank.add_patch(circle2)
    rect_mrd = patches.Rectangle((TC[0]-152.25, TC[1]-131.8), 152.2*2, 131.8 * 2, linewidth=1, edgecolor='black', facecolor='none', linestyle='dashed')
    ax_tank.add_patch(rect_mrd) 
    
    LAPPDsquare = patches.Rectangle((TC[0]-10, TC[1]-10), 20, 20, linewidth=1, edgecolor='red', facecolor='red', alpha=0.3)
    ax_tank.add_patch(LAPPDsquare)

    #ax_tank.set_title('Tank PMT Hits ')
    #ax_tank.set_xlabel('X Coordinate')
    #ax_tank.set_ylabel('Z Coordinate')
    ax_tank.set_xlim(-600, 600)
    ax_tank.set_ylim(-600, 1000)
    
    pmt_text_x = -550
    pmt_text_y1 = 840
    
    pmt_text_fontsize = 12
    pmt_text_foutspace = 40
    ax_tank.text(pmt_text_x, 900, 'ANNIE Phase II', fontsize=20, color='purple', fontweight='bold', ha='left')

    for text_n in range(len(EventInfo)):
        ax_tank.text(pmt_text_x, 840 - pmt_text_foutspace*text_n, EventInfo[text_n], fontsize=pmt_text_fontsize, color='black', ha='left')

    beam_trigger_time_ns = int(EventInfo[-1])  
    beam_trigger_time_s = beam_trigger_time_ns / 1e9  
    utc_time = datetime.datetime.utcfromtimestamp(beam_trigger_time_s)
    central_time = utc_time.astimezone(pytz.timezone('US/Central'))
    formatted_time = central_time.strftime('%Y-%m-%d %H:%M:%S')
    ax_tank.text(pmt_text_x, 840 - pmt_text_foutspace*(len(EventInfo)), formatted_time, fontsize=pmt_text_fontsize, color='black', ha='left')
    

    for text_n in range (len(PMTClusterInfo)):
        ax_tank.text(pmt_text_x, -300 - pmt_text_foutspace*text_n, PMTClusterInfo[text_n], fontsize=pmt_text_fontsize, color='black', ha='left')
    ax_tank.text(pmt_text_x, -300 - pmt_text_foutspace*(len(PMTClusterInfo)), 'PMT plot max PE: {:.2f}'.format(normMax), fontsize=pmt_text_fontsize, color='black', ha='left')
    ax_tank.text(pmt_text_x, -300 - pmt_text_foutspace*(len(PMTClusterInfo)+1), 'PMT plot threshold: 20%', fontsize=pmt_text_fontsize, color='black', ha='left')
    


    
    ####################################################
    #########           plot muons             #########
    ####################################################
    #MRDHits = [MRDhitChankey, MRDhitT]
    #MRDTracks = [MRDTrackStartX, MRDTrackStopX, MRDTrackStartY, MRDTrackStopY, MRDTrackStartZ, MRDTrackStopZ]
    MRD_YZ_Plot_Side = [[],[],[]] # Z is x axis, Y is y axis
    MRD_XZ_Plot_Top = [[],[],[]]  # X is x axis, Z is y axis
    
    for h in range (len(MRDHits[0])):
        if(MRDGeoAccessor.orientation(MRDHits[0][h])==0):
            MRD_YZ_Plot_Side[0].append(MRDGeoAccessor.z_center(MRDHits[0][h]))
            MRD_YZ_Plot_Side[1].append(MRDGeoAccessor.y_center(MRDHits[0][h]))
            MRD_YZ_Plot_Side[2].append(MRDHits[1][h])
        elif (MRDGeoAccessor.orientation(MRDHits[0][h])==1):
            MRD_XZ_Plot_Top[0].append(MRDGeoAccessor.x_center(MRDHits[0][h]))
            MRD_XZ_Plot_Top[1].append(MRDGeoAccessor.z_center(MRDHits[0][h]))
            MRD_XZ_Plot_Top[2].append(MRDHits[1][h])

    # MRD Side view
    if(len(MRD_YZ_Plot_Side[0])>0):
        myz = axmuyz.scatter(MRD_YZ_Plot_Side[0], MRD_YZ_Plot_Side[1], c = MRD_YZ_Plot_Side[2], cmap='viridis', norm=plt.Normalize(vmin=np.min(MRD_YZ_Plot_Side[2]), vmax=np.max(MRD_YZ_Plot_Side[2])), marker='s', s = 40)

        axmuyz.set_xlabel('Z Coordinate')
        axmuyz.set_ylabel('Y Coordinate')
        axmuyz.set_xlim(-150, 600)
        axmuyz.set_ylim(-250, 210)
        axmuyz.set_title('MRD Hit Side View')
        

        tank_side = patches.Rectangle((TC[2]- TRadius, TC[1] - THeight/2), TRadius*2, THeight, linewidth=2, edgecolor='black', facecolor='none')
        axmuyz.add_patch(tank_side) 
        rect_yz = patches.Rectangle((325.5, TC[1]-131.8), 140.49, 131.8 * 2, linewidth=2, edgecolor='black', facecolor='none')
        axmuyz.add_patch(rect_yz)  
        rect_yz_f = patches.Rectangle(( TC[2]-TPMTRadius ,TC[1] - TPMTHeight/2)*100, TPMTRadius*2, TPMTHeight, linewidth=1, edgecolor='blue', facecolor='none', linestyle='dashed')
        axmuyz.add_patch(rect_yz_f)  
        
        cbar1 = plt.colorbar(myz, ax=axmuyz, orientation='horizontal', shrink=0.8, pad = 0)
        cbar1.set_label('MRD Hit Time (ns)', fontsize=8)
        cbar1.ax.tick_params(labelsize=8)
        cbar1.locator = MaxNLocator(integer=True)
        cbar1.update_ticks()

        axmuyz.plot([LAPPD_Center[0][2], LAPPD_Center[0][2]], [LAPPD_Center[0][1] - LAPPD_V_height/2, LAPPD_Center[0][1] + LAPPD_V_height/2], color='red', linewidth=2)
        #print(MRDTracks)
        for t in range (len(MRDTracks[0])):            

            delta_z = MRDTracks[5][t]*100 - MRDTracks[4][t]*100
            delta_y = MRDTracks[3][t]*100 - MRDTracks[2][t]*100
            if hasVeto:
                z_start = -200
            else:
                z_start = 150
            extended_y_start = MRDTracks[2][t]*100 + (z_start - MRDTracks[4][t]*100) * delta_y / delta_z
            extended_y_end = MRDTracks[3][t]*100 + (600 - MRDTracks[5][t]*100) * delta_y / delta_z
            axmuyz.plot([z_start, 600], [extended_y_start, extended_y_end], 'b--', linewidth=1)
            
            axmuyz.plot([MRDTracks[4][t]*100, MRDTracks[5][t]*100], [MRDTracks[2][t]*100, MRDTracks[3][t]*100], 'g-', linewidth=2)

    
    # MRD Top view
    if(len(MRD_XZ_Plot_Top[0])>0):
        mxz = axmuxz.scatter(MRD_XZ_Plot_Top[0], MRD_XZ_Plot_Top[1], c = MRD_XZ_Plot_Top[2], cmap='viridis', norm=plt.Normalize(vmin=np.min(MRD_XZ_Plot_Top[2]), vmax=np.max(MRD_XZ_Plot_Top[2])), marker='s', s = 40)
        
        axmuxz.set_xlabel('X Coordinate')
        axmuxz.set_ylabel('Z Coordinate')
        axmuxz.set_xlim(-300, 300)
        axmuxz.set_ylim(-50, 500)
        axmuxz.set_title('MRD Hit Top View')
        
        circle1 = plt.Circle((TC[0], TC[2]), TRadius, linewidth=2, edgecolor='black', facecolor='none')
        axmuxz.add_patch(circle1)
        circle2 = plt.Circle((TC[0], TC[2]), TPMTRadius, linewidth=1, edgecolor='blue', facecolor='none', linestyle='dashed')
        axmuxz.add_patch(circle2)        
        
        rect_xz = patches.Rectangle((-152.25, 325.5), 152.25*2, 140.49, linewidth=2, edgecolor='black', facecolor='none')
        axmuxz.add_patch(rect_xz)  
        cbar2 = plt.colorbar(mxz, ax=axmuxz, orientation='horizontal', shrink=0.8, pad = 0)
        cbar2.set_label('MRD Hit Time (ns)', fontsize=8)
        cbar2.ax.tick_params(labelsize=8)
        cbar2.locator = MaxNLocator(integer=True)
        cbar2.update_ticks()
        
        axmuxz.plot([LAPPD_Center[0][0] - LAPPD_H_width/2, LAPPD_Center[0][0] + LAPPD_H_width/2], [LAPPD_Center[0][2], LAPPD_Center[0][2]], color='red', linewidth=2)

        for t in range (len(MRDTracks[0])):
            
            delta_x = MRDTracks[1][t]*100 - MRDTracks[0][t]*100
            delta_z = MRDTracks[5][t]*100 - MRDTracks[4][t]*100
            if hasVeto:
                z_start = -100
            else:
                z_start = 150
            extended_x_start = MRDTracks[0][t]*100 + (z_start - MRDTracks[4][t]*100) * delta_x / delta_z
            extended_x_end = MRDTracks[1][t]*100 + (550 - MRDTracks[5][t]*100) * delta_x / delta_z
            axmuxz.plot([extended_x_start, extended_x_end], [z_start, 550], 'b--', linewidth=1)
            
            axmuxz.plot([MRDTracks[0][t]*100, MRDTracks[1][t]*100], [MRDTracks[4][t]*100, MRDTracks[5][t]*100], 'g-', linewidth=2)
         
            
    ####################################################
    #########         plot LAPPD hits          #########
    ####################################################   
    # LAPPDHits = [LAPPDID, LAPPDHitStrip, LAPPDHitTime, LAPPDHitAmp]
    maxPlotTime = 13
    minPlotTime = 8

    maxPlotTime = 25
    minPlotTime = 0
    
    LHit = [[[],[],[]],[[],[],[]],[[],[],[]]]
    # Convert hit time from bin to ns
    for h in range(len(LAPPDHits[0])):
        LHit[LAPPDHits[0][h]][0].append(LAPPDHits[1][h])  # not flip x axis
        LHit[LAPPDHits[0][h]][1].append(LAPPDHits[2][h] / 10)  # time in ns
        LHit[LAPPDHits[0][h]][2].append(LAPPDHits[3][h])       # amp
    # LHit[id][info], where info = strip, time, amp
    
    if len(LHit[0][2]) > 0:
        # Convert LHit times to NumPy array for element-wise comparison
        LHit_strips = np.array(LHit[0][0])  # Strip numbers
        LHit_times = np.array(LHit[0][1])   # Hit times
        LHit_amplitudes = np.array(LHit[0][2])  # Amplitudes
        
        # Apply the mask to filter the times and associated data
        # print("LHit_times:", LHit_times)
        mask = (LHit_times >= minPlotTime) & (LHit_times <= maxPlotTime)
        selected_times = LHit_times[mask]         # Filtered hit times
        minTime = min(selected_times)
        mask2 = (LHit_times >= minTime) & (LHit_times <= minTime+2)

        # Now use the mask to select filtered data
        selected_times = [i - minTime for i in LHit_times[mask2]]      # Filtered hit times
        selected_strips = LHit_strips[mask2]       # Strip numbers corresponding to the time mask
        selected_amplitudes = LHit_amplitudes[mask2]  # Filtered amplitudes        

        if len(selected_times) > 0:
            min_val = np.min(selected_times)
            max_val = np.max(selected_times)
            dT = max_val - min_val
    
            # Set Y-axis limits with a buffer
            y_min = min_val - 0.1 * dT
            y_max = max_val + 0.1 * dT
            if y_min != y_max:
                ax_l0.set_ylim(y_min, y_max)
            else:
                ax_l0.set_ylim(min_val - 0.5, min_val + 0.5)
    
          
            # Create the scatter plot using amplitude data for colors
            l0 = ax_l0.scatter(selected_strips, selected_times,
                               c=selected_amplitudes, cmap = 'YlGnBu', norm=plt.Normalize(vmin=0, vmax=np.max(selected_amplitudes)),
                               s=60, marker='o',
                               edgecolors='face', linewidths=5, alpha=0.8)
            
            # Add colorbar
            cbar0 = plt.colorbar(l0, ax=ax_l0, shrink=0.8)
            cbar0.set_label('Amplitude (mV)')
    
            # Draw a black dashed line at y=0
            ax_l0.axhline(min(selected_times), color='black', linestyle='--', linewidth=1)
    
            # Set axis labels and title
            ax_l0.set_title('Hits on LAPPD ID=0')
            ax_l0.set_xlabel('Strip Number')
            ax_l0.set_ylabel('Hit Time (ns)')
            ax_l0.set_xlim(0, 30)

            dT = max(selected_times) - min(selected_times)

            if(dT>0):
                ax_l0.set_ylim(min(selected_times) - 0.1*dT, max(selected_times) + 0.1*dT)
            else:
                ax_l0.set_ylim(min(selected_times) - 0.5, min(selected_times) + 0.5)

            
            min_time = min(selected_times)
            max_time = max(selected_times)
            dT = max_time - min_time
            

            y_min = - 0.2
            y_max = max_time - min_time + 0.2
            
            # 设置 y 轴范围
            ax_l0.set_ylim(y_min, y_max)
            
            # 生成以 0.2 为间隔的 y 轴刻度，并确保包含 0
            yti = -0.5
            yticks = []
            while(yti<y_max+0.5):
                yticks.append(yti)
                yti += 0.5
            
            # 根据 min_time 生成新的标签，min_time 对应的刻度为 0
            new_yticklabels = [f"{(ytick):.2f}" for ytick in yticks]
            
            # 设置新的 y 轴刻度和标签
            ax_l0.set_yticks(yticks + min_time)
            ax_l0.set_yticklabels(new_yticklabels)

            #print(min_time, max_time, dT, y_min, y_max, yticks, new_yticklabels)
            
    
            # Adjust the position of the plot
            pos = ax_l0.get_position()
            new_pos0 = [pos.x0, pos.y0 + 0.05, pos.width, pos.height * 0.6]
            ax_l0.set_position(new_pos0)
            
        else:
            ax_l0.axis('off')


        
    if(len(LHit[1][2])>0):
        l1 = ax_l1.scatter(LHit[1][0], LHit[1][1], c = LHit[1][2], cmap = 'YlGnBu', norm=plt.Normalize(vmin=0, vmax=np.max(LHit[1][2])),s = 50, marker='o')
        cbar1 = plt.colorbar(l1, ax=ax_l1, shrink=0.8)
        cbar1.set_label('Amplitude (mV)') 
        ax_l1.set_title('Hits on LAPPD ID=1')
        ax_l1.set_xlabel('Strip Number')
        ax_l1.set_ylabel('Hit Time (ns)')
        ax_l1.set_xlim(0, 30)
        ax_l1.set_ylim(8, 13)
        pos = ax_l1.get_position()
        new_pos1 = [pos.x0, pos.y0 + 0.05, pos.width, pos.height * 0.6]
        ax_l1.set_position(new_pos1)
    else:
        ax_l1.axis('off')
        
    if(len(LHit[2][2])>0):
        l2 = ax_l2.scatter(LHit[2][0], LHit[2][1], c = LHit[2][2], cmap = 'YlGnBu', norm=plt.Normalize(vmin=0, vmax=np.max(LHit[2][2])),s = 50, marker='o')
        cbar2 = plt.colorbar(l2, ax=ax_l2, shrink=0.8)
        cbar2.set_label('Amplitude (mV)')
        ax_l2.set_title('Hits on LAPPD ID=2')
        ax_l2.set_xlabel('Strip Number')
        ax_l2.set_ylabel('Hit Time (ns)')
        ax_l2.set_xlim(0, 30)
        ax_l2.set_ylim(8, 13)
        pos = ax_l2.get_position()
        new_pos2 = [pos.x0, pos.y0 + 0.05, pos.width, pos.height * 0.6]
        ax_l2.set_position(new_pos2)   
    else:
        ax_l2.axis('off')                  

                  
    pdf.savefig(fig, bbox_inches = 'tight')
    plt.close(fig)

###################################################################################################################################################################################################################################################



# 设置Decimal的上下文以获得任意精度
decimal.getcontext().prec = 200

pdf = PdfPages(SavePath + pdfName)
printEventNumber = 0

# 获取所有符合模式的文件
file_list = np.sort(glob.glob(file_pattern)) #[:1]

# 存储结果的列表
T_Total = [[],[],[]]
Timing = [[],[],[]]
filtered_events = []


totalNumOfEvent = 0
passCutNumOfEvent = 0


eventNumberInRun = [[],[],[],[],[],[],[],[]] # run number, event number, 

HitNumberPerEvent = []
HitStripPerEvent = []
PulseNumberPerEvent = []
PulseStripPerEvent = []
runNum = []
pfNum = []


for file_name in file_list:
    
    with uproot.open(file_name) as file:
        Event = file["Event"]
        
        #print("Loading:", file_name[60:], ", with events", len(Event))
        print("Loading:", os.path.basename(file_name), ", with events", len(Event))
        
        totalNumOfEvent += len(Event)
        
        runNumber = Event["runNumber"].array()
        partFileNumber = Event["partFileNumber"].array()
        
        GroupedTriggerWord = Event["GroupedTriggerWord"].array()
        GroupedTriggerTime = Event["GroupedTriggerTime"].array()
        PrimaryTriggerWord = Event["PrimaryTriggerWord"].array()
        Extended = Event["Extended"].array()
        beam_ok = Event["beam_ok"].array()
        beam_pot_875 = Event["beam_pot_875"].array()
        
        numberOfClusters = Event["numberOfClusters"].array()
        clusterPE = Event["clusterPE"].array()
        clusterTime = Event["clusterTime"].array()
        clusterChargeBalance = Event["clusterChargeBalance"].array()
        clusterMaxPE = Event["clusterMaxPE"].array()
        Cluster_HitChankey = np.array(Event["Cluster_HitChankey"].array(), dtype=object)
        Cluster_HitPE = np.array(Event["Cluster_HitPE"].array(), dtype=object)
        Cluster_HitX = np.array(Event["Cluster_HitX"].array(), dtype=object)
        Cluster_HitY = np.array(Event["Cluster_HitY"].array(), dtype=object)
        Cluster_HitZ = np.array(Event["Cluster_HitZ"].array(), dtype=object)
        Cluster_HitT = np.array(Event["Cluster_HitT"].array(), dtype=object)
        
        LAPPD_ID = Event["LAPPD_ID"].array()
        LAPPD_Offset = Event["LAPPD_Offset"].array()
        LAPPD_OSInMinusPS = Event["LAPPD_OSInMinusPS"].array()
        LAPPD_BGCorrection = Event["LAPPD_BGCorrection"].array()
        LAPPD_Timestamp_Raw = Event["LAPPD_Timestamp_Raw"].array()
        LAPPD_Beamgate_Raw = Event["LAPPD_Beamgate_Raw"].array()
        LAPPD_TSCorrection = Event["LAPPD_TSCorrection"].array()
        LAPPD_BGPPSBefore = Event["LAPPD_BGPPSBefore"].array()  # in unit of tick
        LAPPD_BGPPSMissing = Event["LAPPD_BGPPSMissing"].array() # in unit of tick
        LAPPD_TSPPSBefore = Event["LAPPD_TSPPSBefore"].array()  # in unit of tick
        LAPPD_TSPPSMissing = Event["LAPPD_TSPPSMissing"].array() # in unit of tick

        LAPPD_PulseIDs = Event["LAPPD_PulseIDs"].array()
        LAPPD_PeakAmp = Event["LAPPD_PeakAmp"].array()
        LAPPD_PulseWidth = Event["LAPPD_PulseWidth"].array()
        LAPPD_PulseSide = Event["LAPPD_PulseSide"].array()
        LAPPD_PulseStripNum = Event["LAPPD_PulseStripNum"].array()
        LAPPD_PeakTime = Event["LAPPD_PeakTime"].array()

        LAPPDID_Hit = Event["LAPPDID_Hit"].array()
        LAPPDHitStrip = Event["LAPPDHitStrip"].array()
        LAPPDHitTime = Event["LAPPDHitTime"].array()
        LAPPDHitAmp = Event["LAPPDHitAmp"].array()
        
        MRDClusterNumber = Event["MRDClusterNumber"].array()
        HasMRD = Event["HasMRD"].array()
        NoVeto = Event["NoVeto"].array()
        TankMRDCoinc = Event["TankMRDCoinc"].array()
        MRDhitChankey = Event["MRDhitChankey"].array()
        MRDhitT = Event["MRDhitT"].array()
        MRDTrackStartX = Event["MRDTrackStartX"].array()
        MRDTrackStartY = Event["MRDTrackStartY"].array()
        MRDTrackStartZ = Event["MRDTrackStartZ"].array()
        MRDTrackStopX = Event["MRDTrackStopX"].array()
        MRDTrackStopY = Event["MRDTrackStopY"].array()
        MRDTrackStopZ = Event["MRDTrackStopZ"].array()

        
        
        RWMRisingStart = Event["RWMRisingStart"].array()
        RWMRisingEnd = Event["RWMRisingEnd"].array()
        RWMHalfRising = Event["RWMHalfRising"].array()
        RWMFirstPeak = Event["RWMFirstPeak"].array()
        
        BRFFirstPeak = Event["BRFFirstPeak"].array()
        BRFFirstPeakFit = Event["BRFFirstPeakFit"].array()
        
        eventTimeTank = Event["eventTimeTank"].array()
        FMVhitChankey = Event["FMVhitChankey"].array()
        NumClusterTracks = Event["NumClusterTracks"].array()
        
        eventNumberThisRun = 0
        vetoEventNumber = 0
        vetoKeyEventNumber = 0
        firstLayerEventNumber = 0
        fmvandFirstLayer = 0
        through = 0
        tracksEvent = 0

        
        prevRunNumber = runNumber[0]
        
        for i in tqdm(range(len(Event))):
            
            #if(i>5000):
            #    break
                
            eventNumberThisRun+=1
            thisRunNum = runNumber[i]
            
            if(thisRunNum>4100 and thisRunNum<4201):
                continue
                
            if(thisRunNum!=prevRunNumber):
                eventNumberInRun[0].append(prevRunNumber)
                eventNumberInRun[1].append(eventNumberThisRun)
                eventNumberInRun[2].append(vetoEventNumber)
                eventNumberInRun[3].append(vetoKeyEventNumber)
                eventNumberInRun[4].append(firstLayerEventNumber)
                eventNumberInRun[5].append(fmvandFirstLayer)
                eventNumberInRun[6].append(through)
                eventNumberInRun[7].append(tracksEvent) 
                eventNumberThisRun = 0
                vetoEventNumber = 0
                vetoKeyEventNumber = 0
                firstLayerEventNumber = 0
                fmvandFirstLayer = 0
                through = 0
                tracksEvent = 0
                
                prevRunNumber = thisRunNum
                
            if(cut_beamOK):
                if(beam_ok[i]!=1):
                    continue
            
            ###################################
            #####  plot for MRD activity  #####
            ###################################
            
            if(NoVeto[i]==0):
                vetoEventNumber+=1 
                if(TankMRDCoinc[i]==1):
                    through+=1
                

            MRDveto = False
            for j in range (len(MRDhitChankey[i])):
                if(MRDhitChankey[i][j]>=0 and MRDhitChankey[i][j]<=25):
                    vetoKeyEventNumber+=1
                    MRDveto = True
                    for j in range (len(MRDhitChankey[i])):
                        if(MRDhitChankey[i][j]>=26 and MRDhitChankey[i][j]<=51):
                            fmvandFirstLayer+=1
                            break
                    break
                    
            if(not MRDveto):
                if(len(FMVhitChankey[i])>0):
                    vetoKeyEventNumber+=1
                    for j in range (len(MRDhitChankey[i])):
                        if(MRDhitChankey[i][j]>=26 and MRDhitChankey[i][j]<=51):
                            fmvandFirstLayer+=1
                            break

                    
            for j in range (len(MRDhitChankey[i])):
                if(MRDhitChankey[i][j]>=26 and MRDhitChankey[i][j]<=51):
                    firstLayerEventNumber+=1
                    break
                    
            for j in range (len(NumClusterTracks[i])):
                if(NumClusterTracks[i][j])>0:
                    tracksEvent+=1
                    break
                    
            ###################################
            ##########   Apply cuts  ##########
            ###################################
        
            if(cut_clusterExist):
                if(numberOfClusters[i]<1):
                    continue
                    
                maxIndex = 0
                maxPE = clusterMaxPE[i][0]
                for x in range(len(clusterMaxPE[i])):
                    if(clusterMaxPE[i][x]>maxPE):
                        maxPE = clusterMaxPE[i][x]
                        maxIndex = x
                    
                if(cut_clusterMaxPE):    
                    if(clusterPE[i][maxIndex]<cut_clusterMaxPE_value):
                        continue
                if(cut_clusterCB):
                    if(clusterChargeBalance[i][maxIndex]>cut_clusterCB_value):
                        continue
                
            if(cut_MRDactivity):
                cut_noVeto = False
                cut_MRDPMTCoinc = False
                cut_hasTrack = False
                
                hasActivity = False
                if any(num > 0 for num in NumClusterTracks[i]) or (NoVeto[i]==0):
                    hasActivity = True
                if(not hasActivity):
                    continue
                    
            if(cut_noVeto):
                if(NoVeto[i]!=1):
                    continue
                    
            if(cut_MRDPMTCoinc):
                if(TankMRDCoinc[i]!=1):
                    continue
                    
            if(cut_hasTrack):
                if all(num <= 0 for num in NumClusterTracks[i]):
                    continue
            
            if(cut_ChrenkovCover):
                PassCut = True
                for ids in LAPPD_ID[i]:
                    nearingPMTID = PMT_chanKey[ids]
                    brightClusterKeyArray = Cluster_HitChankey[i][maxIndex]
                    brightClusterPEArray = Cluster_HitPE[i][maxIndex]
                    
                    nearingPMTKey = [idx for idx, value in enumerate(brightClusterKeyArray) if value in nearingPMTID]
                    if len(nearingPMTKey) == 0:
                        PassCut = False
                        break
                    nearingPMTPE = [brightClusterPEArray[idx] for idx in nearingPMTKey]
                    if sum(pe > cut_ChrenkovCover_PMT_PE for pe in nearingPMTPE) < cut_ChrenkovCover_nPMT:
                        PassCut = False
                        break
                        
                if(not PassCut):
                    continue

            if(cut_LAPPDMultip):
                PassCut = False
                '''
                for j, lappd_id in enumerate(LAPPD_ID[i]):
                    PassHitNum = 0
                    for x, ids in enumerate(LAPPDID_Hit[i]):
                        if(ids == lappd_id and LAPPDHitAmp[i][x]>cut_LAPPDHitAmp):
                            PassHitNum+=1
                    if(PassHitNum>=cut_LAPPDHitNum):
                        PassCut = True
                        
                '''
                for j, lappd_id in enumerate(LAPPD_ID[i]):
                    PassPulseNum = 0
                    for x, ids in enumerate(LAPPD_PulseIDs[i]):
                        if(ids == lappd_id and LAPPD_PeakAmp[i][x]>cut_LAPPDHitAmp and LAPPD_PulseSide[i][x] == 0 and LAPPD_PeakTime[i][x]>16 and LAPPD_PeakTime[i][x]<22):
                            PassPulseNum+=1
                    if(PassPulseNum>=cut_LAPPDHitNum):
                        PassCut = True
                if(not PassCut):
                    continue

            ####################################################################################
            ############   now, this event has past all cuts, time operators  ##################
            ####################################################################################

                
            passCutNumOfEvent+=1
            
            trigger_indices = [index for index, word in enumerate(GroupedTriggerWord[i]) if word == 14]
            beam_trigger_time = GroupedTriggerTime[i][trigger_indices[0]]
            
            # 保存当前事件的数据
            event_data = {
                'clusterPE': clusterPE[i],
                'clusterTime': clusterTime[i],
                'clusterChargeBalance': clusterChargeBalance[i],
                'clusterMaxPE': clusterMaxPE[i],
                'Cluster_HitChankey': Cluster_HitChankey[i],
                'Cluster_HitPE': Cluster_HitPE[i],
                'Cluster_HitX': Cluster_HitX[i],
                'Cluster_HitY': Cluster_HitY[i],
                'Cluster_HitZ': Cluster_HitZ[i],
                'Cluster_HitT': Cluster_HitT[i],
                'LAPPD_ID': LAPPD_ID[i],
                'LAPPD_Timestamp_Raw': LAPPD_Timestamp_Raw[i],
                'LAPPD_Beamgate_Raw': LAPPD_Beamgate_Raw[i],
                'LAPPD_PulseIDs': LAPPD_PulseIDs[i],
                'LAPPD_PeakAmp': LAPPD_PeakAmp[i],
                'LAPPD_PulseWidth': LAPPD_PulseWidth[i],
                'LAPPD_PulseSide': LAPPD_PulseSide[i],
                'LAPPD_PeakTime': LAPPD_PeakTime[i],
                'LAPPDID_Hit': LAPPDID_Hit[i],
                'LAPPDHitStrip': LAPPDHitStrip[i],
                'LAPPDHitTime': LAPPDHitTime[i],
                'LAPPDHitAmp': LAPPDHitAmp[i]
            }
            
            filtered_events.append(event_data)

            #if(len(LAPPDHitStrip[i])>3):
            HitStripPerEvent.append(LAPPDHitStrip[i])
            HitNumberPerEvent.append(len(LAPPDHitStrip[i]))
            PulseNumberPerEvent.append(len(LAPPD_PulseStripNum[i]))
            PulseStripPerEvent.append(LAPPD_PulseStripNum[i])
            runNum.append(runNumber[i])
            pfNum.append(partFileNumber[i])
            
            if(printPDF and printEventNumber<printEventMaxNumber):
                '''
                PMTHits = [hitX, hitY, hitZ, hitT, hitPE]
                MRDHits = [MRDhitChankey, MRDhitT]
                MRDTracks = [MRDTrackStartX, MRDTrackStopX, MRDTrackStartY, MRDTrackStopY, MRDTrackStartZ, MRDTrackStopZ]
                LAPPDHits = [LAPPDID, LAPPDHitStrip, LAPPDHitTime, LAPPDHitAmp]                
                '''
                plotIndex = 0
                plotPE = clusterPE[i][0]
                PMTClusterTime = clusterTime[i][0]
                extendedClusterPE = []
                extendedClusterTime = []
                extendedClusterNum = 0
                
                for e in range (0,len(clusterPE[i])):
                    if(clusterTime[i][e] > 2000):
                        extendedClusterNum+=1
                        extendedClusterPE.append(clusterPE[i][e])
                        extendedClusterTime.append(clusterTime[i][e])
                    if(clusterPE[i][e] > plotPE and clusterTime[i][e] < 2000):
                        plotIndex = e
                        plotPE = clusterPE[i][e]
                        PMTClusterTime = clusterTime[i][e]

                
                
                EventInfo = [
                    'Event: '+str(passCutNumOfEvent),
                    'Run: ' + str(runNumber[i]), 
                    'Part File: ' + str(partFileNumber[i]), 
                    'Beam Quality: '+ str(beam_ok[i]),
                    'Has veto: ' + str(NoVeto[i]==0),
                    'Extended: ' + str(Extended[i]),
                    'PMT Hit number: '+str(len(Cluster_HitX[i][plotIndex])),
                    'LAPPD Hit number: '+str(len(LAPPDID_Hit[i])),
                    'Primary Trigger Word: '+str(PrimaryTriggerWord[i]),
                    'Trigger Time (Central): ',
                    str(beam_trigger_time)
                ]
                
                PMTClusterInfo = [
                    'Cluster: '+str(plotIndex+1)+' in '+str(len(clusterTime[i])),
                    'Extended Cluster: '+str(extendedClusterNum),
                    'Cluster Time: {:.2f} ns'.format(PMTClusterTime),
                    'Cluster PE: {:.2f}'.format(plotPE),
                    'Cluster Charge Balange: {:.2f}'.format(clusterChargeBalance[i][plotIndex])
                ]
                
                PMTHits = [Cluster_HitX[i][plotIndex], Cluster_HitY[i][plotIndex], Cluster_HitZ[i][plotIndex], Cluster_HitT[i][plotIndex], Cluster_HitPE[i][plotIndex],Cluster_HitChankey[i][plotIndex]]
                ExtendedCluster = [extendedClusterTime, extendedClusterPE]
                MRDHits = [[],[]]
                for mh in range (len(MRDhitT[i])):
                    dT = PMTClusterTime - MRDhitT[i][mh]
                    if(dT>-800 and dT<-300):
                        MRDHits[1].append(MRDhitT[i][mh])
                        MRDHits[0].append(MRDhitChankey[i][mh])
                
                MRDTracks = [MRDTrackStartX[i], MRDTrackStopX[i], MRDTrackStartY[i], MRDTrackStopY[i], MRDTrackStartZ[i], MRDTrackStopZ[i]]
                
                LAPPDHitsPlot = [LAPPDID_Hit[i], LAPPDHitStrip[i], LAPPDHitTime[i], LAPPDHitAmp[i]]
                
                #LAPPDHitsPlot = [[],[],[],[]]
                
                EventDisplay(pdf, EventInfo, PMTClusterInfo, PMTHits, ExtendedCluster, MRDHits, MRDTracks, LAPPDHitsPlot, NoVeto[i]==0)
                printEventNumber+=1


                
            for j, lappd_id in enumerate(LAPPD_ID[i]):
                T_Total[lappd_id].append(Decimal((int(LAPPD_Timestamp_Raw[i][j]) - int(LAPPD_Beamgate_Raw[i][j]))) * Decimal(3.125))
                Timing[lappd_id].append(Decimal((int(LAPPD_Timestamp_Raw[i][j]) - int(LAPPD_Beamgate_Raw[i][j]))) * Decimal(3.125))

                
print('Total number of loaded events:', totalNumOfEvent)
print('Total number of events pass cuts:', passCutNumOfEvent)


print('Saved')
pdf.close()
