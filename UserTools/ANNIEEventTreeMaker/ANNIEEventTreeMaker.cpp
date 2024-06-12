#include "ANNIEEventTreeMaker.h"

ANNIEEventTreeMaker::ANNIEEventTreeMaker():Tool(){}


bool ANNIEEventTreeMaker::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool ANNIEEventTreeMaker::Execute(){

  return true;
}


bool ANNIEEventTreeMaker::Finalise(){

  return true;
}
