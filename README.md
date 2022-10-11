# 512ClosedLoop
This Program is meant to be used in conjuction with the 512 Multi-Electrode Array created by Alan Litke.
The purpose of this software is to run analysis on the live data and display the data in a way that is easily and quickly interpretable.


## Features
###### **Oscilliscope Panel:**  
See the raw voltage data from any electrode, as well as the mean and threshold used for spike finding.


###### **Spikes Panel:**  
See when and where spikes occur on a map of all of the electrodes. Spikes are represented by circles on the electrode they were detected.
The size of the circle indicates the amplitude of spike. The circle fades over time.

###### **Post-Stimulus Time Histograms:**
*Only available if you are stimulating on electrodes*

Displays the PSTH for a specified stimulation sequence, summarizing spikes found on either all channels or individual channels.

To see, check the 'Stim?' Box on first screen, and choose the stimulus file you are using. 
512ClosedLoop will find every time a unique sequence of stimulation pulses will occur, then record any spikes occuring within 1 second after each pulse and
update the PSTH.
