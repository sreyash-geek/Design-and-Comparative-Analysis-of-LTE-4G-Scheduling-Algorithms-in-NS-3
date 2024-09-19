Commands to run the program:


Step 1: Copy the file NWSassign2.cc in the ns3’s scratch folder
Step 2: In terminal move to ns-3.40 folder and run below commands
        
./ns3 configure
./ns3 build
./ns3 run "NWSassign1 --speed=0 --RngRun=15 --fullBufferFlag=true --scheType=3"


– speed -> provides speed to UE’s
– RngRun -> sets seed value
– fullBufferFlag -> bool value, specify if the buffer should be full or not
–scheType -> set the schedule type PF=0,RR=1,MT=2,BETS=3




Commands to plot the graphs:


Step 1: Open google collab
Step 2: Upload all the files from the folder Plotting_Data
Step 3: Upload notebook name PlotGraph.ipynb
Step 4: Run all the cells




Command to plot rem


Step 1: Open terminal and write gnuplot 
Step 2:
set terminal png size 640,480
set output "Rem.png"
set title "Radio Environment Map"
set xlabel "X"
set ylabel "Y"
set cblabel "SINR (dB)"
unset key
plot "rem.out" using ($1):($2):(10*log10($4)) with image