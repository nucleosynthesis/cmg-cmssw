//
// Package:         RecoTracker/RoadSearchCloudMaker
// Class:           RoadSearchCloudCleanerAlgorithm
// 
// Description:     
//
// Original Author: Steve Wagner, stevew@pizero.colorado.edu
// Created:         Sat Feb 19 22:00:00 UTC 2006
//
// $Author: gutsche $
// $Date: 2007/07/10 03:14:44 $
// $Revision: 1.7 $
//

#include <vector>

#include "RecoTracker/RoadSearchCloudCleaner/interface/RoadSearchCloudCleanerAlgorithm.h"

#include "DataFormats/RoadSearchCloud/interface/RoadSearchCloud.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

RoadSearchCloudCleanerAlgorithm::RoadSearchCloudCleanerAlgorithm(const edm::ParameterSet& conf) {
  
  // store parameter
  mergingFraction_ = conf.getParameter<double>("MergingFraction");
  maxRecHitsInCloud_ = conf.getParameter<int>("MaxRecHitsInCloud");

}

RoadSearchCloudCleanerAlgorithm::~RoadSearchCloudCleanerAlgorithm() {
}

void RoadSearchCloudCleanerAlgorithm::run(const RoadSearchCloudCollection* input,
					  const edm::EventSetup& es,
					  RoadSearchCloudCollection &output)
{

  //
  //  right now cloud cleaning solely consist of merging clouds based on the number
  //  of shared hits (getting rid of obvious duplicates) - don't need roads and
  //  geometry for this - eventually this stage will become a sub-process (probably
  //  early on) of cloud cleaning
  //

  LogDebug("RoadSearch") << "Raw Clouds input size: " << input->size(); 

  //
  //  no raw clouds - nothing to try merging
  //

  if ( input->empty() ){
    LogDebug("RoadSearch") << "Found " << output.size() << " clouds.";
    return;  
  }

  //
  //  1 raw cloud - nothing to try merging, but one cloud to duplicate
  //

  if ( 1==input->size() ){
    output = *input;
    LogDebug("RoadSearch") << "Found " << output.size() << " clouds.";
    return;
  }  

  //
  //  got > 1 raw cloud - something to try merging
  //
  std::vector<bool> already_gone(input->size());
  for (unsigned int i=0; i<input->size(); ++i) {
    already_gone[i] = false; 
  }

  int raw_cloud_ctr=0;
  // loop over clouds
  for ( RoadSearchCloudCollection::const_iterator raw_cloud = input->begin(); raw_cloud != input->end(); ++raw_cloud) {
    ++raw_cloud_ctr;

    if (already_gone[raw_cloud_ctr-1])continue;

    // produce output cloud where other clouds are merged in
    RoadSearchCloud lone_cloud = *raw_cloud;
    int second_cloud_ctr=raw_cloud_ctr;
    for ( RoadSearchCloudCollection::const_iterator second_cloud = raw_cloud+1; second_cloud != input->end(); ++second_cloud) {
      second_cloud_ctr++;

      std::vector<const TrackingRecHit*> unshared_hits;

      if ( already_gone[second_cloud_ctr-1] )continue;

      for ( RoadSearchCloud::RecHitVector::const_iterator second_cloud_hit = second_cloud->begin_hits();
	    second_cloud_hit != second_cloud->end_hits();
	    ++ second_cloud_hit ) {
	bool is_shared = false;
	for ( RoadSearchCloud::RecHitVector::const_iterator lone_cloud_hit = lone_cloud.begin_hits();
	      lone_cloud_hit != lone_cloud.end_hits();
	      ++ lone_cloud_hit ) {

	  if ((*lone_cloud_hit)->geographicalId().rawId() == (*second_cloud_hit)->geographicalId().rawId())
	    if ((*lone_cloud_hit)->localPosition().x() == (*second_cloud_hit)->localPosition().x())
	      if ((*lone_cloud_hit)->localPosition().y() == (*second_cloud_hit)->localPosition().y())
		{is_shared=true; break;}
	}
	if (!is_shared)  unshared_hits.push_back(*second_cloud_hit);

	if ( ((float(unshared_hits.size())/float(lone_cloud.size())) > 
	      ((float(second_cloud->size())/float(lone_cloud.size()))-mergingFraction_)) &&
	     ((float(unshared_hits.size())/float(second_cloud->size())) > (1-mergingFraction_))){
	  // You'll never merge these clouds..... Could quit now!
	  break;
	}

      }
      
      double f_lone_shared=double(second_cloud->size()-unshared_hits.size())/double(lone_cloud.size());
      double f_second_shared=double(second_cloud->size()-unshared_hits.size())/double(second_cloud->size());
      
      if ( ( (static_cast<unsigned int>(f_lone_shared*1E9) > static_cast<unsigned int>(mergingFraction_*1E9))||(static_cast<unsigned int>(f_second_shared*1E9) > static_cast<unsigned int>(mergingFraction_*1E9)) ) 
	   && (lone_cloud.size()+unshared_hits.size() <= maxRecHitsInCloud_) ){

	LogDebug("RoadSearch") << " Merge CloudA: " << raw_cloud_ctr << " with  CloudB: " << second_cloud_ctr 
			       << " Shared fractions are " << f_lone_shared << " and " << f_second_shared;

	//
	//  got a cloud to merge
	//
	for (unsigned int k=0; k<unshared_hits.size(); ++k) {
	  lone_cloud.addHit(unshared_hits[k]);
	}

	already_gone[second_cloud_ctr-1]=true;

	}//end got a cloud to merge

    }//interate over all second clouds

    output.push_back(lone_cloud);
    
  }//iterate over all raw clouds

  LogDebug("RoadSearch") << "Found " << output.size() << " clean clouds.";

}


