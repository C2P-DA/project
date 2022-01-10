# C2P-DA (Contrasting Consecutive Pattern Discovery Algorithm)

## Read before use
   - Code runs on Windows x64 machines
   - Both (i) the source code and (ii) an executable compiled with Visual Studio's Release Build are provided in the root folder
   - This README file provides the instructions for the execution and its configuration

## Instructions
1) Configure the execution by editing the config.json file
2) Build the code folder's content and run the executable
   OR
   Run C2P-DA.exe

## Configuration example for Anomaly Detection searches
{  
&nbsp;&nbsp;&nbsp;&nbsp;// Execution start date and time configuration  
&nbsp;&nbsp;&nbsp;&nbsp;"year": "2022",  
&nbsp;&nbsp;&nbsp;&nbsp;"month": "01",  
&nbsp;&nbsp;&nbsp;&nbsp;"day": "01",  
&nbsp;&nbsp;&nbsp;&nbsp;"hours": "00",  
&nbsp;&nbsp;&nbsp;&nbsp;"minutes": "00",  
&nbsp;&nbsp;&nbsp;&nbsp;"seconds": "00",  
  
&nbsp;&nbsp;&nbsp;&nbsp;// The equivalents of q<sub>m</sub> and q<sub>M</sub>  
&nbsp;&nbsp;&nbsp;&nbsp;"minSubwindow": "157",  
&nbsp;&nbsp;&nbsp;&nbsp;"maxSubwindow": "157",  
  
&nbsp;&nbsp;&nbsp;&nbsp;// Leave as it is  
&nbsp;&nbsp;&nbsp;&nbsp;"approximationEnabled": false,  
&nbsp;&nbsp;&nbsp;&nbsp;"partialSearch": true,  
&nbsp;&nbsp;&nbsp;&nbsp;"dukascopyStockInputData": false,  
  
&nbsp;&nbsp;&nbsp;&nbsp;// Leave as it is (β will be automatically set to q)  
&nbsp;&nbsp;&nbsp;&nbsp;"ifApproximationEnabled": {  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;// The equivalent of η  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"differenceBetweenSubwindows": "1000",  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;// The equivalent of β  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"beta": "128",  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"startEndDislocationLimit": "1 / 3",  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"maxPeaksPerPattern": "3",  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"minPeakWidth": "1000",  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"pointsDislocationLimit": "1 / 8",  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"minConf": "2 / 3",  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"heightAffectsConf": true,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"distanceAffectsConf": true,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"greedySigmaEnabled": true  
&nbsp;&nbsp;&nbsp;&nbsp;},  
  
&nbsp;&nbsp;&nbsp;&nbsp;// Leave as it is  
&nbsp;&nbsp;&nbsp;&nbsp;"ifNotGreedySigmaEnabled": {  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"knn": "1"  
&nbsp;&nbsp;&nbsp;&nbsp;},  
  
&nbsp;&nbsp;&nbsp;&nbsp;// Leave as it is  
&nbsp;&nbsp;&nbsp;&nbsp;"ifPartialSearch": {  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;// The equivalent of µ  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"storageWindow": "2"  
&nbsp;&nbsp;&nbsp;&nbsp;},  
  
&nbsp;&nbsp;&nbsp;&nbsp;// The folder that contains the input files (all the files included in the folder will be analyzed)  
&nbsp;&nbsp;&nbsp;&nbsp;"dataPath": "data/MIT-BIH Record 100 (V5)/",  
&nbsp;&nbsp;&nbsp;&nbsp;// The results folder  
&nbsp;&nbsp;&nbsp;&nbsp;"logsPath": "logs/MIT-BIH Record 100 (V5)/",  
&nbsp;&nbsp;&nbsp;&nbsp;// The folder where execution times will be stored (grouped by and averaged for 1000 data points)  
&nbsp;&nbsp;&nbsp;&nbsp;"ratesPath": "rates/MIT-BIH Record 100 (V5)/"  
}
