//- Reading ISAT parameters
Switch isatCheck(combustionOptionsDictionary.subDict("ISAT").lookup("ISAT"));
ISAT *isatTable;
Eigen::VectorXd scalingFactors_ISAT;
int luSolver_ISAT = 1;
label numberSubSteps_ISAT = 1;

if (isatCheck == true)
{
	// mandatory if ISAT ON
	scalar epsilon_ISAT = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<double>("tolerance", 1e-4);
	       numberSubSteps_ISAT = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<int>("numberSubSteps", 1);

	scalar maxSizeMRU = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<int>("maxSizeMRU", 100);
	scalar maxSizeMFU = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<int>("maxSizeMFU", 100);
	scalar maxSizeBT = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<int>("maxSizeBT", 100000);
	scalar maxSearchMRU = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<int>("maxSearchMRU", 10);
	scalar maxSearchMFU = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<int>("maxSearchMFU", 30);
	Switch searchMRU(combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<Switch>("searchMRU", true)); 
	Switch searchMFU(combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<Switch>("searchMFU", true));
	Switch searchBF(combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<Switch>("searchBruteForce", false));
	Switch clearIfFull(combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<Switch>("clearingIfFull", false));
	Switch cleanAndBalance(combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<Switch>("cleanAndBalance", true));
	scalar maxTimeOldCoeff = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<double>("maxTimeOldCoeff", 0.7);
	scalar maxGrowCoeff = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<double>("maxGrowCoeff", 0.5);
	scalar maxHeightCoeff = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<double>("maxHeightCoeff", 20.);
	scalar minUsedCoeff = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<double>("minUsedCoeff", 0.01);
	scalar balanceFactorRetrieve = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<double>("balanceFactorRetrieve", 2.);
	scalar balanceFactorAddition = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<double>("balanceFactorAddition", 0.1);
	word   luFactorization = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<word>("luFactorization","Partial");
	word   qrFactorization = combustionOptionsDictionary.subDict("ISAT").lookupOrDefault<word>("qrFactorization","Full");

	if (luFactorization != "Partial" && luFactorization != "Full")
	{
		Info << "Wrong luFactorization options: Partial || Full" << endl;
		abort();
	} 
	else 
	{
		if (luFactorization == "Full") 
			luSolver_ISAT = 0;
		else
			luSolver_ISAT = 1;		
	}

	int qrSolver_ISAT = 0;
	if (qrFactorization != "NoPivoting" && qrFactorization != "Partial" && qrFactorization != "Full")
	{
		Info << "Wrong qrFactorization options: NoPivoting || Partial || Full" << endl;
		abort();
	} 
	else 
	{
		if (qrFactorization == "Full") 
			qrSolver_ISAT = 0;
		else if (qrFactorization == "Partial") 
			qrSolver_ISAT = 1;		
		else if (qrFactorization == "NoPivoting")
			qrSolver_ISAT = 2;
	}


	//- ISAT scale factor 
	scalingFactors_ISAT.setConstant(thermodynamicsMapXML->NumberOfSpecies()+2,1.);
	if(isatCheck == true) 
	{
		const unsigned int NC  = thermodynamicsMapXML->NumberOfSpecies();

		//massFractionsTol = epsilon_ISAT;
	
		dictionary scalingFactors(combustionOptionsDictionary.subDict("ISAT").subDict("scalingFactors"));
		for(unsigned int i=0;i<NC;i++) 
		{	
			if(scalingFactors.found(thermodynamicsMapXML->NamesOfSpecies()[i])) 
				scalingFactors_ISAT(i) = 1./readScalar(scalingFactors.lookup(thermodynamicsMapXML->NamesOfSpecies()[i]));
			else 
				scalingFactors_ISAT(i) = 1./readScalar(scalingFactors.lookup("others"));
		}
		scalingFactors_ISAT(NC)   = 1./readScalar(scalingFactors.lookup("T"));
		scalingFactors_ISAT(NC+1) = 1./readScalar(scalingFactors.lookup("tf"));
	}
	//- ISAT scaling error factors
	Eigen::VectorXd scalingErrors_ISAT;
	scalingErrors_ISAT.setConstant(thermodynamicsMapXML->NumberOfSpecies()+2,1.);
	if(isatCheck == true) 
	{
		const unsigned int NC  = thermodynamicsMapXML->NumberOfSpecies();
	
		dictionary scalingErrors(combustionOptionsDictionary.subDict("ISAT").subDict("scalingErrors"));
	
		for(unsigned int i=0;i<NC;i++) 
		{
			if(scalingErrors.found(thermodynamicsMapXML->NamesOfSpecies()[i])) 
				scalingErrors_ISAT(i) = readScalar(scalingErrors.lookup(thermodynamicsMapXML->NamesOfSpecies()[i]));
			else 
				scalingErrors_ISAT(i) = readScalar(scalingErrors.lookup("others"));
		}
		scalingErrors_ISAT(NC)   = readScalar(scalingErrors.lookup("T"));
		scalingErrors_ISAT(NC+1) = readScalar(scalingErrors.lookup("tf"));
	}

	//- ISAT HOM
	isatTable = new ISAT(scalingErrors_ISAT, epsilon_ISAT, thermodynamicsMapXML->NumberOfSpecies()+2);	

	// - Setting ISAT_HOM param
	isatTable->setMaxSizeBT(maxSizeBT);
	isatTable->setMaxSizeMRU(maxSizeMRU);
	isatTable->setMaxSizeMFU(maxSizeMFU);
	isatTable->setMaxSearchMRU(maxSearchMRU);
	isatTable->setMaxSearchMFU(maxSearchMFU);
	isatTable->setFlagSearchMRU(searchMRU);
	isatTable->setFlagSearchMFU(searchMFU);
	isatTable->setFlagClearingIfFull(clearIfFull);
	isatTable->setMaxGrowCoeff(maxGrowCoeff);
	isatTable->setMaxHeightCoeff(maxHeightCoeff);
	isatTable->setMaxTimeOldCoeff(maxTimeOldCoeff);
	isatTable->setMinUsedCoeff(minUsedCoeff);
	isatTable->setBalanceFactorRet(balanceFactorRetrieve);
	isatTable->setBalanceFactorAdd(balanceFactorAddition);
	isatTable->setQRType(qrSolver_ISAT);
	isatTable->setFlagCleanAndBalance(cleanAndBalance);

	{
		const unsigned int NC  = thermodynamicsMapXML->NumberOfSpecies();
	
		Info << endl << "ISAT parameters " << endl;
		Info << "   tolerance           : " << epsilon_ISAT << endl;
		Info << "   luFactorization     : " << luFactorization << endl; 	
		Info << "   qrFactorization     : " << qrFactorization << endl; 	
	
		Info << "   scalingFactors      : " << endl;
		for(unsigned int i=0;i<NC;i++)  
		{
			if(scalingFactors_ISAT(i) != 1.) 
				Info << "      " << thermodynamicsMapXML->NamesOfSpecies()[i] << "\t       : " << scalingFactors_ISAT(i) << endl;
		}
		Info << "       others          : " << readScalar(combustionOptionsDictionary.subDict("ISAT").subDict("scalingFactors").lookup("others")) << endl;
		Info << "       T               : " << scalingFactors_ISAT(NC) << endl;


		Info << "   clear if BT full    : " << clearIfFull << endl;

		Info << "   search in MRU       : " << searchMRU << endl;
		Info << "   search in MFU       : " << searchMFU << endl;
		Info << "   search BF           : " << searchBF << endl;

		Info << "   dimension parameters: " << endl;
		Info << "      max size BT      : " << maxSizeBT << endl;	
		if(searchMRU == true) 
		{
			Info << "      max size MRU     : " << maxSizeMRU << endl;
			Info << "      max search MRU   : " << maxSearchMRU << endl;
		}
		if(searchMFU == true) 
		{
			Info << "      max size MFU     : " << maxSizeMRU << endl;
			Info << "      max search MFU   : " << maxSearchMFU << endl;  
		}
	
		if (cleanAndBalance == true)
		{
			Info << "   balance parameters  : " << endl;
			Info << "      balanceFactorRetrieve   : " << balanceFactorRetrieve << endl;	
			Info << "      balanceFactorAddition   : " << balanceFactorAddition << endl;	
			Info << "      maxHeightCoeff          : " << maxHeightCoeff << endl;	
			Info << "      maxGrowCoeff            : " << maxGrowCoeff << endl;
			Info << "      minUsedCoeff            : " << minUsedCoeff << endl;
			Info << "      maxTimeOldCoeff         : " << maxTimeOldCoeff << endl; 
		}

		Info << "   scaling error       : " << endl;
		for(unsigned int i=0;i<NC;i++)  
		{
			if(scalingErrors_ISAT(i) != 1.) 
				Info << "      " << thermodynamicsMapXML->NamesOfSpecies()[i] << "\t       : " << scalingErrors_ISAT(i) << endl;
		}
		Info << "       others          : " << readScalar(combustionOptionsDictionary.subDict("ISAT").subDict("scalingErrors").lookup("others")) << endl;
		Info << "       T               : " << scalingErrors_ISAT(NC) << endl;
	
		Info << endl;
	}

	if (	odeParametersFineStructures.type() != OpenSMOKE::ODE_Parameters::ODE_INTEGRATOR_OPENSMOKE &&
		isatCheck == true)
	{
		Info << "In the current version of edcSMOKE the ISAT can be used only together with the native OpenSMOKE Integrator." << endl;
		abort();
	}

	if (	reactorType == REACTORTYPE_PSR &&
		isatCheck == true)
	{
		Info << "In the current version of edcSMOKE the ISAT can be used only together with the PFR model (fine structures)." << endl;
		abort();
	}
}



