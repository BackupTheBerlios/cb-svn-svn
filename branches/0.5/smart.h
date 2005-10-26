// This class should more or less do the same as std::auto_ptr
// Except I somehow always get the syntax wrong with std::auto_ptr
//
// Copyright (C) 2002 Thomas Denk
//
// Borrowed for the Code::Blocks SVN Plugin
// Copyright (C) 2005 Thomas Denk
//
// This program is licensed under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
// $HeadURL$
// $Id$


#ifndef INC_SMART_H
#define INC_SMART_H


template<class T>
class smart
  {
  protected:
    T* ptr;

  public:
    smart()
    {
      ptr = 0;
    }

    smart(T *p)
    {
      ptr = 0;
      *this = p;
    }

    smart(const smart<T> &p)
    {
      ptr = 0;
      *this = p;
    }

    ~smart()
    {
      if(ptr)
        delete ptr;
    }

    bool operator =(T *p)
    {
      if(ptr)
        delete ptr;
      ptr = p;
      return true;
    }

    bool operator =(const smart<T> &p)
    {
      if(ptr)
        delete ptr;
      ptr=p.ptr;
      return true;
    }

    T& operator *() const
      {
        assert(ptr != 0);
        return *ptr;
      }

    T* operator ->() const
      {
        assert(ptr != 0);
        return ptr;
      }

    operator T*() const
      {
        return ptr;
      }

    bool Valid() const
      {
        return (ptr != 0);
      }

    bool Null() const
      {
        return (ptr == 0);
      }

    bool operator !()
    {
      return !(ptr);
    }

    bool operator ==(const smart<T> &p) const
      {
        return (ptr == p.ptr);
      }

    bool operator ==(const T* p) const
      {
        return (ptr == p);
      }
  };

#endif


