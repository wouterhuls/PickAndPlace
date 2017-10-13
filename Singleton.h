#ifndef SINGLETON_H
#define SINGLETON_H

namespace PAP
{
  template<typename T>
    class Singleton
  {
  public:
    ~Singleton() {
      if(g_instance==this) g_instance = 0 ;
    }
    
    static T* instance() {
      // this can be made thread safe:
      // http://www.aristeia.com/Papers/DDJ%5FJul%5FAug%5F2004%5Frevised.pdf
      if(g_instance==0) g_instance = new T() ;
      return g_instance ;
    }
  protected:
    Singleton() {}
  private:
    static T* g_instance;
  } ;

  template<typename T> T* Singleton<T>::g_instance=0 ;
  
}

#endif
