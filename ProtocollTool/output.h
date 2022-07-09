#pragma once
#include <ostream>
#include <streambuf>

class Redirecter
{
public:
    // Constructing an instance of this class causes
    // anything written to the source stream to be redirected
    // to the destination stream.
    Redirecter(std::ostream& dst, std::ostream& src)
        : src(src)
        , srcbuf(src.rdbuf())
    {
        src.rdbuf(dst.rdbuf());
    }

    // The destructor restores the original source stream buffer
    ~Redirecter()
    {
        src.rdbuf(srcbuf);
    }
private:
    std::ostream& src;
    std::streambuf* const srcbuf;
};