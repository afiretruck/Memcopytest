#include "arguments.h"

using namespace std;

//---------------------------------------------------------------------------
// Argument exception stuff
//---------------------------------------------------------------------------

ArgumentException::ArgumentException(const string& msg)
    : exception()
    , m_Message(msg)
{

}

const string ArgumentException::WhatHappened() const
{
    return m_Message;
}

//---------------------------------------------------------------------------
// Argument processing
//---------------------------------------------------------------------------

string Arguments::GetHelpString()
{
    return  "Required parameters:"
            "\n\t-b <gigabytes> The size of the source and destination buffers in gigabytes."
            "\n\t-i <iterations> The number of test iterations to perform."
            "\n\t-w <thread count> The number of thraeds to use."
            "\n\t-t <copy programme> The name of the copy programme to use. Choose from:"
            "\n\t                    boring - uses the c standard memcpy and memcmp using a single thread."
            "\n\t                    hugepages - same as above, but allocate memory using huge pages.";
}

Arguments::Arguments(const int argc, const char** argv)
{
    // Go through arguments, looking for key-value pairs.
    // Chuck out an ArgumentException if a problem comes up.

    // First argument is the path of this executable.
    // So there should be an odd number of parameters.
    if(argc < 5)
    {
        throw ArgumentException("Incorrect number of arguments specified.");
    }

    for(int i = 1; i < argc; i += 2)
    {
        string key = argv[i];

        if(key == "-b")
        {
            int bufferSize = atoi(argv[i + 1]);
            if(bufferSize <= 0)
            {
                throw ArgumentException("Buffer size " + string(argv[i + 1]) + " is invalid.");
            }
            else if(bufferSize > 2048)
            {
                throw ArgumentException("Do you *really* have more than 2TB of memory?");
            }
            else
            {
                m_BufferSizeGB = static_cast<uint32_t>(bufferSize);
            }
        }
        else if(key == "-i")
        {
            int iterations = atoi(argv[i + 1]);
            if(iterations <= 0)
            {
                throw ArgumentException("Iterations count " + string(argv[i + 1]) + " is invalid.");
            }
            else
            {
                m_Iterations = static_cast<uint32_t>(iterations);
            }
        }
        else if(key == "-t")
        {
            string programmeStr = argv[i + 1];
            if(programmeStr == "boring")
            {
                m_Programme = TestProgramme::Boring;
            }
            else if(programmeStr == "hugepages")
            {
                m_Programme = TestProgramme::HugePages;
            }
            else
            {
                throw ArgumentException("Test " + programmeStr + " is not recognised.");
            }
        }
        else if(key == "-w")
        {
            int threadCount = atoi(argv[i + 1]);
            if(threadCount <= 0)
            {
                throw ArgumentException("Thread count " + string(argv[i + 1]) + " is invalid.");
            }
            else
            {
                m_Threads = static_cast<uint32_t>(threadCount);
            }
        }
        else
        {
            throw ArgumentException("Parameter switch " + key + " is not recognised.");
        }
        
    }

    if(m_BufferSizeGB == 0 || m_Programme == TestProgramme::None)
    {
        throw ArgumentException("Please provide a buffer size and a copy programme to test.");
    }
}

uint32_t Arguments::GetBufferSizeGB() const
{
    return m_BufferSizeGB;
}

Arguments::TestProgramme Arguments::GetProgramme() const
{
    return m_Programme;
}

uint32_t Arguments::GetIterations() const
{
    return m_Iterations;
}

uint32_t Arguments::GetThreads() const
{
    return m_Threads;
}