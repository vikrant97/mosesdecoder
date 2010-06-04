/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>

#include "FeatureVector.h"
#include "Hypothesis.h"
#include "ScoreProducer.h"
#include "TranslationOptionCollection.h"


using namespace Moses;
namespace po = boost::program_options;

namespace Moses {
  class ScoreProducer;
  class ScoreIndexManager;
}


namespace Josiah {

  class Sample;


/**
* Represents a gap in the target sentence. During score computation, this is used to specify where the proposed
* TranslationOptions are to be placed. The left and right hypothesis are to the left and to the right in target order.
* Note that the leftHypo could be the null hypo (if at start) and the rightHypo could be a null pointer (if at end).
**/
struct TargetGap {
  TargetGap(const Hypothesis* lh, const Hypothesis* rh, const WordsRange& s) :
      leftHypo(lh), rightHypo(rh), segment(s) {
      //check that they're in target order.
      assert(!lh->GetPrevHypo() || lh->GetCurrTargetWordsRange() < s);
      assert(!rh || s < rh->GetCurrTargetWordsRange());
  }
  
  const Hypothesis* leftHypo;
  const Hypothesis* rightHypo;
  WordsRange segment;
};


/**
  * Base class for Gibbler feature functions.
   * The FeatureFunction operations are called as folloews:
   *  1. When Gibbler starts up, and initialises feature functions:
   *      - constructor
   *  2. When a new Sample() object is created to begin sampling on a new sentence:
   *     - init() - with the new sample
   *     - updateTarget() - to indicate to the FeatureFunction that the target words have changed
   *     - assignScore() -  to tell the FeatureFunction to calculate its initial score
   *  3. When scoring possible transitions.
   *     -  doXXX() - to calculate the score deltas.
   *  4. When performing a transition.
   *    - updateTarget() - called with new target words. For paired updates, this is called twice, and after the first 
   *                        call the feature_vector (in the sample) will be inconsistent with the target words
 **/
class FeatureFunction {
  public:
    FeatureFunction() {}
    /** Initialise with new sample */
    virtual void init(const Sample& sample) {/* do nothing */};
    /** Update the target words.*/
    virtual void updateTarget(){/*do nothing*/}
    
    /** Assign the total score of this feature on the current hypo */
    virtual void assignScore(FVector& scores) = 0;
    
    /** Score due to one segment */
    virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores) = 0;
    /** Score due to two segments. The left and right refer to the target positions.**/
    virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap, FVector& scores) = 0;
    virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) = 0;
    
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) = 0;
    
    /** Checks the internal consistency of the FeatureFunction by computing the scores from scratch and comparing with the 
     given scores. */
    bool isConsistent(const FVector& featureValues);
    
    virtual ~FeatureFunction() = 0;
    
};

/** 
  * A feature function with a single value
  **/
class SingleValuedFeatureFunction: public FeatureFunction {
  public:
    SingleValuedFeatureFunction(const std::string& rootName) :
      m_name(rootName,"") {}
    virtual void assignScore(FVector& scores)
      {scores[m_name] = computeScore();}
    virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores)
      {scores[m_name] =  getSingleUpdateScore(option,gap);}
    virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                          const TargetGap& gap, FVector& scores)
    {scores[m_name] = getContiguousPairedUpdateScore(leftOption,rightOption,gap);}
    virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                             const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores)
    {scores[m_name] = getDiscontiguousPairedUpdateScore(leftOption,rightOption,leftGap,rightGap);}
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                              const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores)
    {scores[m_name] = getFlipUpdateScore(leftOption,rightOption,leftGap,rightGap);}
    
    /**
      * Actual feature functions need to implement these methods.
    **/
  protected:
    virtual FValue computeScore() = 0;
    /** Score due to one segment */
    virtual FValue getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) = 0;
    /** Score due to two segments. The left and right refer to the target positions.**/
    virtual FValue getContiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
       const TargetGap& gap) = 0;
    virtual FValue getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap) = 0;
    
    /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
    virtual FValue getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap) = 0;
    
    virtual ~SingleValuedFeatureFunction() {}
    
  private:
    FName m_name;
  
    
};


typedef boost::shared_ptr<FeatureFunction> feature_handle;
typedef std::vector<feature_handle> feature_vector;




} //namespace
