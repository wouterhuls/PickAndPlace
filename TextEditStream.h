#ifndef TEXTEDITSTREAM_H
#define TEXTEDITSTREAM_H

#include <QPlainTextEdit>
#include <sstream>

namespace PAP
{
  // wrapper class that allows to stream to a qtextedit using
  // stringstream. perhaps we could template this.
  class TextEditStream
  {
  private:
    QPlainTextEdit* m_textbox ;
  public:
    TextEditStream( QPlainTextEdit& textbox) : m_textbox(&textbox) {}
    template<typename T>
    TextEditStream& operator<<( const T& text) {
      std::stringstream os ;
      os << text ;
      m_textbox->appendPlainText( os.str().c_str() ) ;
      return *this ;
    }
  };
}

#endif
