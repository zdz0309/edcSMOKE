/*----------------------------------------------------------------------*\
|                  _       _____ __  __  ____  _  ________                |
|                 | |     / ____|  \/  |/ __ \| |/ /  ____|               |
|          ___  __| | ___| (___ | \  / | |  | | ' /| |__                  |
|         / _ \/ _` |/ __|\___ \| |\/| | |  | |  < |  __|                 |
|        |  __/ (_| | (__ ____) | |  | | |__| | . \| |____                |
|         \___|\__,_|\___|_____/|_|  |_|\____/|_|\_\______|               |
|                                                                         |
|                                                                         |
|   Authors: A. Cuoci, M.R. Malik, A. Parente                             |
|                                                                         |
|   Contacts: Alberto Cuoci                                               |
|   email: alberto.cuoci@polimi.it                                        |
|   Department of Chemistry, Materials and Chemical Engineering           |
|   Politecnico di Milano                                                 |
|   P.zza Leonardo da Vinci 32, 20133 Milano (Italy)                      |
|                                                                         |
|   Contacts: Mohammad Rafi Malik                                         |
|   Aero-Thermo-Mechanical Department                                     |
|   Université Libre de Bruxelles                                         |
|   Avenue F. D. Roosevelt 50, 1050 Bruxelles (Belgium)                   |
|                                                                         |
|   Contacts: Alessandro Parente                                          |
|   email: alessandro.parente@ulb.ac.be                                   |
|   Aero-Thermo-Mechanical Department                                     |
|   Université Libre de Bruxelles                                         |
|   Avenue F. D. Roosevelt 50, 1050 Bruxelles (Belgium)                   |
|                                                                         |
|-------------------------------------------------------------------------|
|                                                                         |
|   This file is part of edcSMOKE solver.                                 |
|                                                                         |
|	License                                                           |
|                                                                         |
|   Copyright(C) 2014 A. Cuoci, M.R. Malik, A. Parente                    |
|   edcSMOKE is free software: you can redistribute it and/or modify      |
|   it under the terms of the GNU General Public License as published by  |
|   the Free Software Foundation, either version 3 of the License, or     |
|   (at your option) any later version.                                   |
|                                                                         |
|   edcSMOKE is distributed in the hope that it will be useful,           |
|   but WITHOUT ANY WARRANTY; without even the implied warranty of        |
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         |
|   GNU General Public License for more details.                          |
|                                                                         |
|   You should have received a copy of the GNU General Public License     |
|   along with edcSMOKE. If not, see <http://www.gnu.org/licenses/>.      |
|                                                                         |
\*-----------------------------------------------------------------------*/

{
    volScalarField rAU(1.0/UEqn().A());
    volVectorField HbyA("HbyA", U);
    HbyA = rAU*UEqn().H();

    UEqn.clear();

    bool closedVolume = false;

    if (simple.transonic())
    {
        surfaceScalarField phid
        (
            "phid",
            fvc::interpolate(psi)
           *(fvc::interpolate(HbyA) & mesh.Sf())
        );

        fvOptions.makeRelative(fvc::interpolate(psi), phid);

        while (simple.correctNonOrthogonal())
        {
            fvScalarMatrix pEqn
            (
                fvm::div(phid, p)
              - fvm::laplacian(rho*rAU, p)
              ==
                fvOptions(psi, p, rho.name())
            );

            // Relax the pressure equation to ensure diagonal-dominance
            pEqn.relax();

            fvOptions.constrain(pEqn);

            pEqn.setReference(pRefCell, pRefValue);

            pEqn.solve();

            if (simple.finalNonOrthogonalIter())
            {
                phi == pEqn.flux();
            }
        }
    }
    else
    {
        surfaceScalarField phiHbyA
        (
            "phiHbyA",
            fvc::interpolate(rho*HbyA) & mesh.Sf()
        );

        fvOptions.makeRelative(fvc::interpolate(rho), phiHbyA);

        closedVolume = adjustPhi(phiHbyA, U, p);

        while (simple.correctNonOrthogonal())
        {
            fvScalarMatrix pEqn
            (
                fvc::div(phiHbyA)
              - fvm::laplacian(rho*rAU, p)
              ==
                fvOptions(psi, p, rho.name())
            );

            pEqn.setReference(pRefCell, pRefValue);

            fvOptions.constrain(pEqn);

            pEqn.solve();

            if (simple.finalNonOrthogonalIter())
            {
                phi = phiHbyA + pEqn.flux();
            }
        }
    }


    #include "incompressible/continuityErrs.H"

    // Explicitly relax pressure for momentum corrector
    p.relax();

    U = HbyA - rAU*fvc::grad(p);
    U.correctBoundaryConditions();
    fvOptions.correct(U);

    // For closed-volume cases adjust the pressure and density levels
    // to obey overall mass continuity
    if (closedVolume)
    {
        p += (initialMass - fvc::domainIntegrate(psi*p))
            /fvc::domainIntegrate(psi);
    }

    rho = thermo.rho();
    rho = max(rho, rhoMin);
    rho = min(rho, rhoMax);

    if (!simple.transonic())
    {
        rho.relax();
    }

    Info<< "rho max/min : "
        << max(rho).value() << " "
        << min(rho).value() << endl;
}
