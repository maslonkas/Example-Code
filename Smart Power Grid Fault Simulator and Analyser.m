%{
Smart Power Grid Fault Simulator and Analyser
Written by StanLee Maslonka

Program Purpose:	This program uses the Z Bus Building Algorithm to accomplish the following:
					(1) To simulate fault scenarios in order to predict the effects of a faulted transmission line 
					on a city power-grid.
					(2) In the event of a faulted transmission line, operators may use this program to calculate
					where power is being directed and determine potential electrical hazards.
Inputs:	A text file ('inputDeck') containing an information deck of the city's power grid is automatically 
		updated by a separate program.  In theory, the below program may accept and evaluate a city power 
		grid of any (unlimited) size as long as the data in the information deck is accurate, but in reality the 
		performance	of this program is limited by the processing power of the workstation running it.  The GUI will 
		prompt the operator to input values (Fault location & Impedance at fault) to calculate the resulting current
		voltages on the transmission lines.
Outputs:	Displays input data, calculated fault current, bus voltages during fault, and resulting line current.
%}

clc
clear

% Inputs text file & read in initializing values
filein=fopen('inputDeck.txt','r');
NumBus=fscanf(filein,'%g',1);
NumLine=fscanf(filein,'%g',1);
NumGen=fscanf(filein,'%g',1);
BaseMVA=fscanf(filein,'%g',1);

% Zeros all matrices being used for calculations 
BusLookup=zeros(NumBus,1);
oldBus=zeros(NumBus,1);
FromBus=zeros(NumLine,1);
ToBus=zeros(NumLine,1);
ArrayLookup=zeros(NumBus,1);
LineDone=zeros(NumLine,1);
deltaZ=zeros(NumBus,1);
VBus=zeros(NumBus,1);
Zbus=zeros(NumBus,NumBus);

% Rule 1 Impedance to Ground Nodes (Read in Generator Buses)
fprintf('Three-Phase Fault Calculation\n\n');
fprintf('Generator & Line Data\n');
fprintf('  From\t\tTo\t\tR\t\tX\n');
for n=1:NumGen	% Displays the resistance and reactance of transmission lines between generator buses
    BusLookup(n)=fscanf(filein,'%g',1);
    fprintf('\t %g\t\t',BusLookup(n));
    ArrayLookup(BusLookup(n))=n;
    dummy=fscanf(filein,'%g',1);
    fprintf('%g\t',dummy);
    r=fscanf(filein,'%g',1);
    fprintf('%6.2f\t',r);
    x=fscanf(filein,'%g',1);
    fprintf('%6.2f\n',x);
    Zgen(n,n)=r+1j*x;	% Combines resistance and reactance for the impedance of each generator
    oldBus(BusLookup(n))=1;
end

% Rule 2 Old to New Nodes (Read in Lines)
arrayLoc=NumGen;
for n=1:NumLine
    FromBus(n)=fscanf(filein,'%g',1);
    fprintf('\t %g\t\t',FromBus(n));
    ToBus(n)=fscanf(filein,'%g',1);
    fprintf('%g\t',ToBus(n));
    R=fscanf(filein,'%g',1);
    fprintf('%6.2f\t',R);
    X=fscanf(filein,'%g',1);
    fprintf('%6.2f\n',X);
    ZLine(n)=R+1j*X;	% Combines resistance and reactance for the impedance of each transmission line
    if oldBus(FromBus(n)) && ~oldBus(ToBus(n))
        arrayLoc=arrayLoc+1;
        for m=1:NumBus
            Zbus(arrayLoc,m)=Zbus(ArrayLookup(FromBus(n)),m);
            Zbus(m,arrayLoc)=Zbus(m,ArrayLookup(FromBus(n)));
        end
		% Create the impedance Z Bus matrix
        Zbus(arrayLoc,arrayLoc)=Zbus(ArrayLookup(FromBus(n)),ArrayLookup(FromBus(n)))+ZLine(n);
        oldBus(ToBus(n))=1;
        BusLookup(arrayLoc)=ToBus(n);
        ArrayLookup(ToBus(n))=arrayLoc;
        LineDone(n)=1;
    end
end

% Rule 3 Old Nodes (Add Lines)
for n=1:NumLine
    if LineDone(n)==0
        arrayLoc=arrayLoc+1;
        for m=1:NumBus
			% Calculate the difference of impedance
            deltaZ(m)=Zbus(m,ArrayLookup(FromBus(n)))-Zbus(m,ArrayLookup(ToBus(n)));
        end
		% Calculate the parallel impedance
        zll=ZLine(n)+Zbus(ArrayLookup(ToBus(n)),ArrayLookup(ToBus(n)))-2*Zbus(ArrayLookup(FromBus(n)),ArrayLookup(ToBus(n)));
        % Subtract from existing matrix
		Zsub=1/zll*(deltaZ*deltaZ.');
        Zbus=Zbus-Zsub;
    end
end

% Select the location of the fault
faultedBus=input('\nFault at bus -> ');
% Input the impedance of the fault type
zFault=input('\nImpedence at fault -> ');
% Calculate the fault current
Ifault=1.0/(Zbus(ArrayLookup(faultedBus))+zFault);
% Displays fault current
fprintf('\nI fault = %6.4f per unit\n',Ifault);

% Displays bus voltages during the fault
fprintf('\nPer Unit Bus Voltages during Fault\n');
fprintf('   Bus\t\tMagnitude\t Angle');
for n=1:NumBus	% Uses Ohm's Law calculates the voltage and phase at each bus
    VBus(n)=1.0-Ifault*Zbus(ArrayLookup(n),ArrayLookup(faultedBus));
    fprintf('\n\t %g\t\t  %6.4f\t%6.4f',n,abs(VBus(n)),angle(VBus(n))*180/pi);
end
for n=1:NumGen	% Ohm's Law calculates the voltage and phase using fault current at each generator
    VGen(n)=1.0-Ifault*Zgen(n,n);
end

% Displays the resulting current across transmission lines
fprintf('\n\nPer Unit Line Current for a Fault at Bus %g\n', faultedBus);
fprintf('  From\t   To\t  Magnitude\t  Angle\n');
for n=1:NumGen	% Ohm's Law calculates the current and phase through each generator
    Igen(n)=(0-VGen(n))/Zgen(n,n);
    fprintf('\t %g\t %g\t  %6.4f\t %6.4f \n',0,BusLookup(n),abs(Igen(n)),(angle(Igen(n))*180/pi));
end
for n=1:NumLine	% Ohm's Law calculate the current and phase through each transmission line
    ILine(n)=(VBus(FromBus(n))-VBus(ToBus(n)))/ZLine(n);
    fprintf('\t %g\t   %g  \t  %6.4f\t%6.4f \n',FromBus(n),ToBus(n),abs(ILine(n)),(angle(ILine(n))*180/pi));
end

fclose('all'); % THE END