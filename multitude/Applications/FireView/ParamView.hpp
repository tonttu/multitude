/* COPYRIGHT
 *
 * This file is part of Applications/FireView.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Applications/FireView.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef FIREVIEW_PARAMVIEW_HPP
#define FIREVIEW_PARAMVIEW_HPP

#include <QtGui/QWidget>

class QLabel;
class QSlider;


namespace FireView {
  
  class CamView;

  class Mapper : public QObject
  {
    Q_OBJECT;
    
  public:
    Mapper(QObject * parent, int val)
      : QObject(parent), m_val(val) {}
      
  public slots:
    void setInt(int i) { emitInt(m_val ,i); }
  signals:
    void emitInt(int,int);
  private:
    int m_val;
  };
  
  class ParamView : public QWidget
  {
    Q_OBJECT;
  public:

    ParamView(CamView *);
    virtual ~ParamView();

    void init();

  public slots:

    void sliderMoved(int,int);
    void setAuto(int,int);

  protected:

    class RowWidgets {
    public:
      RowWidgets() : slider(0), value(0) {}

      QSlider * slider;
      QLabel  * value;
    };
    
    std::vector<RowWidgets> m_rows;

    CamView * m_camview;
  };
  
}

#endif
