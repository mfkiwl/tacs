/*
  This file is part of TACS: The Toolkit for the Analysis of Composite
  Structures, a parallel finite-element code for structural and
  multidisciplinary design optimization.

  Copyright (C) 2010 University of Toronto
  Copyright (C) 2012 University of Michigan
  Copyright (C) 2014 Georgia Tech Research Corporation
  Additional copyright (C) 2010 Graeme J. Kennedy and Joaquim
  R.R.A. Martins All rights reserved.

  TACS is licensed under the Apache License, Version 2.0 (the
  "License"); you may not use this software except in compliance with
  the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0
*/

#include "TACSAverageTemperature.h"
#include "TACSAssembler.h"

/*
  Initialize the TACSAverageTemperature class properties
*/
TACSAverageTemperature::TACSAverageTemperature( TACSAssembler *_assembler ):
  TACSFunction(_assembler, TACSFunction::ENTIRE_DOMAIN,
               TACSFunction::SINGLE_STAGE, 0){
  volume = 0.0;
  integral_temp = 0.0;
}

TACSAverageTemperature::~TACSAverageTemperature(){}

const char *TACSAverageTemperature::funcName = "TACSAverageTemperature";

const char *TACSAverageTemperature::getObjectName(){
  return funcName;
}

/*
  Retrieve the function value
*/
TacsScalar TACSAverageTemperature::getFunctionValue(){
  return integral_temp/volume;
}

/*
  Set the volume and integral of temperature to zero on all MPI processes
*/
void TACSAverageTemperature::initEvaluation( EvaluationType ftype ){
  integral_temp = 0.0;
  volume = 0.0;
}

/*
  Sum the volume and integral of temperature across all MPI processors
*/
void TACSAverageTemperature::finalEvaluation( EvaluationType ftype ){
  // Distribute the values of the KS function computed on this domain
  TacsScalar temp[2], result[2];
  temp[0] = volume;
  temp[1] = integral_temp;
  MPI_Allreduce(temp, result, 2, TACS_MPI_TYPE,
                MPI_SUM, assembler->getMPIComm());
  volume = result[0];
  integral_temp = result[1];
}

/*
  Evaluate the temperature contributed by this element
*/
void TACSAverageTemperature::elementWiseEval( EvaluationType ftype,
                                              int elemIndex,
                                              TACSElement *element,
                                              double time,
                                              TacsScalar scale,
                                              const TacsScalar Xpts[],
                                              const TacsScalar vars[],
                                              const TacsScalar dvars[],
                                              const TacsScalar ddvars[] ){
  // Retrieve the basis object for this element (if it exists)
  TACSElementBasis *basis = element->getElementBasis();

  if (basis){
    for ( int i = 0; i < basis->getNumQuadraturePoints(); i++ ){
      double pt[3];
      double weight = basis->getQuadraturePoint(i, pt);

      // Evaluate the strain energy density
      TacsScalar temp = 0.0;
      int count = element->evalPointQuantity(elemIndex, TACS_TEMPERATURE,
                                             time, i, pt,
                                             Xpts, vars, dvars, ddvars, &temp);

      if (count >= 1){
        // Evaluate the determinant of the Jacobian
        TacsScalar Xd[9], J[9];
        TacsScalar detJ = basis->getJacobianTransform(i, pt, Xpts, Xd, J);

        volume += detJ*weight;
        integral_temp += detJ*weight*temp;
      }
    }
  }
}

/*
  These functions are used to determine the sensitivity of the
  function to the state variables.
*/
void TACSAverageTemperature::getElementSVSens( int elemIndex,
                                               TACSElement *element,
                                               double time,
                                               TacsScalar alpha,
                                               TacsScalar beta,
                                               TacsScalar gamma,
                                               const TacsScalar Xpts[],
                                               const TacsScalar vars[],
                                               const TacsScalar dvars[],
                                               const TacsScalar ddvars[],
                                               TacsScalar dfdu[] ){
  // Zero the derivative of the function w.r.t. the element state
  // variables
  int numVars = element->getNumVariables();
  memset(dfdu, 0, numVars*sizeof(TacsScalar));

  // Retrieve the basis object for this element (if it exists)
  TACSElementBasis *basis = element->getElementBasis();

  if (basis){
    for ( int i = 0; i < basis->getNumQuadraturePoints(); i++ ){
      double pt[3];
      double weight = basis->getQuadraturePoint(i, pt);

      // Evaluate the average temperature
      TacsScalar temp = 0.0;
      int count = element->evalPointQuantity(elemIndex, TACS_TEMPERATURE,
                                             time, i, pt,
                                             Xpts, vars, dvars, ddvars, &temp);

      if (count >= 1){
        // Evaluate the determinant of the Jacobian
        TacsScalar Xd[9], J[9];
        TacsScalar detJ = basis->getJacobianTransform(i, pt, Xpts, Xd, J);

        // Evaluate the derivative of the temperature w.r.t. states
        TacsScalar dfdq = detJ*weight/volume;
        element->addPointQuantitySVSens(elemIndex, TACS_TEMPERATURE, time,
                                        alpha, beta, gamma,
                                        i, pt, Xpts, vars, dvars, ddvars,
                                        &dfdq, dfdu);
      }
    }
  }
}

/*
  Retrieve the element contribution to the derivative of the function
  w.r.t. the element nodes
*/
void TACSAverageTemperature::getElementXptSens( int elemIndex,
                                                TACSElement *element,
                                                double time,
                                                TacsScalar scale,
                                                const TacsScalar Xpts[],
                                                const TacsScalar vars[],
                                                const TacsScalar dvars[],
                                                const TacsScalar ddvars[],
                                                TacsScalar dfdXpts[] ){
  // Zero the derivative of the function w.r.t. the element node
  // locations
  int numNodes = element->getNumNodes();
  memset(dfdXpts, 0, 3*numNodes*sizeof(TacsScalar));
}

/*
  Evaluate the derivative of the average temperature w.r.t. the material
  design variables
*/
void TACSAverageTemperature::addElementDVSens( int elemIndex,
                                               TACSElement *element,
                                               double time,
                                               TacsScalar scale,
                                               const TacsScalar Xpts[],
                                               const TacsScalar vars[],
                                               const TacsScalar dvars[],
                                               const TacsScalar ddvars[],
                                               int dvLen, TacsScalar dfdx[] ){
  // Retrieve the basis object for this element (if it exists)
  TACSElementBasis *basis = element->getElementBasis();

  if (basis){
    for ( int i = 0; i < basis->getNumQuadraturePoints(); i++ ){
      double pt[3];
      double weight = basis->getQuadraturePoint(i, pt);

      // Evaluate the strain energy density
      TacsScalar U0 = 0.0;
      int count = element->evalPointQuantity(elemIndex, TACS_TEMPERATURE,
                                             time, i, pt,
                                             Xpts, vars, dvars, ddvars, &U0);

      if (count >= 1){
        // Evaluate the determinant of the Jacobian
        TacsScalar Xd[9], J[9];
        TacsScalar detJ = basis->getJacobianTransform(i, pt, Xpts, Xd, J);

        // Evaluate the derivative of the strain energy
        TacsScalar dfdq = detJ*weight/volume;
        element->addPointQuantityDVSens(elemIndex, TACS_TEMPERATURE,
                                        time, scale, i, pt,
                                        Xpts, vars, dvars, ddvars,
                                        &dfdq, dvLen, dfdx);
      }
    }
  }
}