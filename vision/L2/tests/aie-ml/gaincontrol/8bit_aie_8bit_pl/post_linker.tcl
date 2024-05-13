set CurrentPath [pwd]

set BasePath [regsub {\/L[123]\/.*$} [pwd] ""]
set script [regsub {\/L[123]\/.*$} [pwd] ""]/ext/xf_rtl_utils.tcl

source -notrace $script

#Instances of PL kernels need to be declared as in system.cfg
set TilerInstances [list [list "Tiler_top_1" 0]]
set StitcherInstances [list [list "stitcher_top_1" 0]]

# ConfigDM <HostPixelType> <AIEPixelType> <DDRDataWidth> <AIEDataWidth> <TilerInst> <StitcherInst>
# Default     <XF_8UC1>      <XF_8UC1>        <128>          <128>          <"">         <"">
# If No TilerInst/StitcherInst is used then assign it with "" 
#
# Tiler Instances Configuration 
foreach inst_info $TilerInstances {
    configTiler \
        TilerInstInfo $inst_info
    }

# Stitcher Instances Configuration 
foreach inst_info $StitcherInstances { 
    configStitcher \
        StitcherInstInfo $inst_info
}