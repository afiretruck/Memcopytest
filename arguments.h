#pragma once

#include <exception>
#include <map>
#include <string>

class ArgumentException : public std::exception
{
public:
    ArgumentException(const std::string& msg);

    const std::string WhatHappened() const;

private:
    const std::string m_Message;
};

class Arguments
{
public:
    Arguments(const int argc, const char** argv);

    static std::string GetHelpString();

    enum class TestProgramme
    {
        None,
        Boring,
        HugePages
    };
    TestProgramme GetProgramme() const;
    uint32_t GetBufferSizeGB() const;
    uint32_t GetIterations() const;
    uint32_t GetThreads() const;

private:
    uint32_t m_BufferSizeGB = 0;
    uint32_t m_Iterations = 10;
    uint32_t m_Threads = 1;
    TestProgramme m_Programme = TestProgramme::None;

};