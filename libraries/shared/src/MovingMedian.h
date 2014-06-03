
#ifndef hifi_MovingMedian_h
#define hifi_MovingMedian_h


class MovingMedian {

public:
    MovingMedian(int numSamples = 11);
    ~MovingMedian();

    void updateMedian(float sample);
    float getMedian() const { return _median; }


private:
    float* _samplesSorted;
    int _numSamples;

    int* _sampleAges;   // _sampleAges[i] is the "age" of the sample _sampleSorted[i] (higher means older)
    int _numExistingSamples;
    
    float _median;
};

#endif