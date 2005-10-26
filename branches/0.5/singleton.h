// This file is part of the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
// (actually, this specific class is borrowed from an older project, so should be more like (c) 2001)
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// $HeadURL$
// $Id$


#ifndef __SINGLETON_H__
#define __SINGLETON_H__

template <typename T>
class Singleton
{
public:
    static T* Instance()
    {
        static T instance;
        return &instance;
    };
    
protected:

    Singleton()
    {}
    ;
    virtual ~Singleton()
    {}
    ;
    
private:
    Singleton(const Singleton& source)
    {}
    ;
    
};

#endif
