
#ifndef hifi_MovingPercentile_h
#define hifi_MovingPercentile_h


class MovingPercentile {

public:
    MovingPercentile(int numSamples, float percentile = 0.5f);
    ~MovingPercentile();

    void updatePercentile(float sample);
    float getValueAtPercentile() const { return _valueAtPercentile; }


private:
    const int _numSamples;
    const float _percentile;

    float* _samplesSorted;
    int* _sampleAges;   // _sampleAges[i] is the "age" of the sample at _sampleSorted[i] (higher means older)
    int _numExistingSamples;
    
    float _valueAtPercentile;

    int _indexOfPercentile;
};

#endif