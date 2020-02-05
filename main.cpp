#include <iostream>
#include <vector>
#include "arguments.h"
#include <chrono>
#include <cstring>
#include <thread>
#include <iomanip>
#include <unistd.h>
#include <atomic>
#include <sys/mman.h>
#include <functional>

using namespace std;

static size_t PAGESIZE = 4096;
static const size_t CHUNKSIZE = 1024 * 256 * 1;
static const size_t ONE_GB = 0x40000000;

int main(int argc, const char** argv)
{
    size_t bufferSize;
    Arguments::TestProgramme programme;
    uint32_t iterations;
    uint32_t threadCount;

    try
    {
        Arguments args(argc, argv);
        bufferSize = args.GetBufferSizeGB() * ONE_GB;
        programme = args.GetProgramme();
        iterations = args.GetIterations();
        threadCount = args.GetThreads();
    }
    catch(const ArgumentException& e)
    {
        cerr << e.WhatHappened() << endl;
        cerr << Arguments::GetHelpString() << endl;
        return -1;
    }

    char* pSrc = nullptr;
    char* pDst = nullptr;

    cout << "buffer size 0x" << hex << bufferSize << " (" << dec << bufferSize << ")" << endl;
    cout << "performing " << iterations << " iterations." << endl;

    // allocate the source buffer & destination buffers
    if(programme == Arguments::TestProgramme::HugePages)
    {
        cout <<  "using huge pages." << endl;
        PAGESIZE *= 512;
    }
    else
    {
        cout << "using standard allocation." << endl;
    }

    if(posix_memalign(reinterpret_cast<void**>(&pSrc), PAGESIZE, bufferSize) != 0)
    {
        cout << "allocation failure" << endl;
        return -1;
    }

    if(posix_memalign(reinterpret_cast<void**>(&pDst), PAGESIZE, bufferSize) != 0)
    {
        cout << "allocation failure" << endl;
        return -1;
    }

    if(programme == Arguments::TestProgramme::HugePages)
    {
        if(madvise(pSrc, bufferSize, MADV_HUGEPAGE) != 0)
        {
            cout << "madvise error " << errno <<endl;            
        }
        if(madvise(pDst, bufferSize, MADV_HUGEPAGE) != 0)
        {
            cout << "madvise error " << errno << endl;
        }
    }

    // (for debugging)
    cout << "raw src ptr: 0x" << hex << reinterpret_cast<size_t>(pSrc) << endl;
    cout << "raw dst ptr: 0x" << hex << reinterpret_cast<size_t>(pDst) << endl;
    cout << "------------------------------------------------------" << endl;
    
    // fill in the source buffer with dummy data (this forces page allocations ahead of the copy)
    cout << "Setting the source buffer contents... " << flush;
    auto timerStart = chrono::high_resolution_clock::now();

    memset(pSrc, 27, bufferSize); // 27 because I'm 27

    auto timerEnd = chrono::high_resolution_clock::now();
    double durationSeconds = chrono::duration<double, std::milli>(timerEnd - timerStart).count() / 1000.0;
    cout << "done (" << setprecision(4) << durationSeconds << "s)" << endl;

    
    // useful things
    double totalDurationAverageSeconds = 0.0;

    // threads, chunk counters and things
    vector<thread> threads;
    const uint32_t totalChunks = static_cast<uint32_t>(bufferSize / CHUNKSIZE);
    cout << "Total chunks " << dec << totalChunks << endl;
    atomic_uint32_t chunksCounter = totalChunks + 1;
    atomic_bool diePlease = false;

    // function to copy chunks, using the above counter to reference each chunk. returns when no more chunks are available
    function<void()> copyChunks = [&chunksCounter, totalChunks, pSrc, pDst]()
    {
        while(1)
        {
            // grab a chunk to copy
            uint32_t chunkIndex = chunksCounter++;
            if(chunkIndex >= totalChunks)
            {
                break;
            }

            // generate src and dest pointers
            char* pChunkSrc = pSrc + (chunkIndex * CHUNKSIZE);
            char* pChunkDst = pDst + (chunkIndex * CHUNKSIZE);

            // copy!
            memcpy(pChunkDst, pChunkSrc, CHUNKSIZE);
        }
    };

    // spawn threads (n - 1)
    // These will spin a little bit, but not very much as the main thread is a screaming loop, also.
    // In this case spinning is preferable to sleeping, as sleeping can introduce nasty latency spikes on thread wake.
    // (and different platforms provide different levels of fidelity when specifying the minimum sleep time)
    // I wouldn't want to use this exact method in production code.
    for(uint32_t t = 0; t < threadCount - 1; t++)
    {
        threads.emplace_back([&copyChunks, &diePlease, &chunksCounter, totalChunks, pSrc, pDst]()
        {
            // loop until told to die
            while(diePlease.load() == false)
            {
                copyChunks();
            }
        });
    }
    

    // The main event. Loop for each iteration; start a timer, reset the chunk counter, assist in chunk copying 
    // until none are left, stop the timer, record. Rinse and repeat until all iterations are complete.
    for(uint32_t i = 0; i < iterations; i++)
    {
        // reset the data in the destination buffer
        cout << "Iteration " << dec << i << ": reset destination buffer... " << flush;
        timerStart = chrono::high_resolution_clock::now();

        memset(pDst, 24, bufferSize); // 24 because my wife is 24

        timerEnd = chrono::high_resolution_clock::now();
        durationSeconds = chrono::duration<double, std::milli>(timerEnd - timerStart).count() / 1000.0;
        cout << "done (" << durationSeconds << "s)" << flush;

        // begin copy!
        cout << "\tCopying... " << flush;
        timerStart = chrono::high_resolution_clock::now();

        chunksCounter.store(0);
        copyChunks();

        timerEnd = chrono::high_resolution_clock::now();
        
        durationSeconds = chrono::duration<double, std::milli>(timerEnd - timerStart).count() / 1000.0;
        totalDurationAverageSeconds += durationSeconds;
        double copySpeed = (static_cast<double>(bufferSize) / durationSeconds) / static_cast<double>(ONE_GB);
        cout << "done (" << durationSeconds << "s)\t(" << copySpeed << "GB/s) " << flush;


        // verify the source matches the destination
        cout << "Checking destination copy... " << flush;
        if(memcmp(pSrc, pDst, bufferSize) != 0)
        {
            cout << "Source and destination do not match!" << endl;
        }
        cout << "done" << endl;

    }

    // After all iterations are complete, kill the worker threads and wait for them to join.
    diePlease.store(true);
    for(thread& t : threads)
    {
        t.join();
    }

    cout << "------------------------------------------------------" << endl;
    cout << "Total time taken for tests: " << setprecision(3) << totalDurationAverageSeconds << "s" << endl;
    cout << "Average iteration time: " << setprecision(3) << totalDurationAverageSeconds / static_cast<double>(iterations) << "s" << endl;

    free(pSrc);
    free(pDst);

    return 0;
}