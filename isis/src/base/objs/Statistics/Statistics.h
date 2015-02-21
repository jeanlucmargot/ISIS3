#ifndef Statistics_h
#define Statistics_h
/**
 * @file
 * $Date: 2010/03/19 20:34:55 $
 * $Revision: 1.5 $
 *
 *  Unless noted otherwise, the portions of Isis written by the USGS are public domain. See
 *  individual third-party library and package descriptions for intellectual property information,
 *  user agreements, and related information.
 *
 *  Although Isis has been used by the USGS, no warranty, expressed or implied, is made by the
 *  USGS as to the accuracy and functioning of such software and related material nor shall the
 *  fact of distribution constitute any such warranty, and no responsibility is assumed by the
 *  USGS in connection therewith.
 *
 *  For additional information, launch $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *  in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *  http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *  http://www.usgs.gov/privacy.html.
 */

#include <QObject>

#include "Constants.h"
#include "SpecialPixel.h"
#include "XmlStackedHandler.h"

class QDataStream;
class QUuid;
class QXmlStreamWriter;

namespace Isis {
  class Project;// ??? does xml stuff need project???
  class XmlStackedHandlerReader;
 /**
  * @brief This class is used to accumulate statistics on double arrays.
  *
  * This class is used to accumulate statistics on double arrays. In
  * particular, it is highly useful for obtaining statistics on cube data.
  * Parameters which can be computed are 1) @b average, 2) @b standard
  * @b deviation, 3) @b variance, 4) @b minimum, 5) @b maximum and 6)
  * @b various @b counts of valid and/or special pixels.
  *
  * The following example shows a simple set up and usage of the Statistics
  * class to calculate the average of a set of values:
  *
  * @code
  *   Statistics myStats ;
  *   double myData [] = { 1.0, 3.0, 2.4, 7.5 } ;
  *
  *   myStats.AddData (myData, 4) ;
  *   double myAverage = myStats.Average () ;
  *   cout << "The average of the data is " << myAverage << endl ;
  * @endcode
  *
  * For an example of how the Statistics object is used in %Isis, see the
  * Histogram object (inherits from Statistics) and the stats application,
  * stats.cpp (uses the Statistics child class Histogram).
  *
  * @ingroup Math
  * @ingroup Statistics
  *
  * @author 2002-05-06 Jeff Anderson
  *
  * @internal
  *   @history 2002-05-08 Jeff Anderson - Added Chebyshev and Best minimum/maximum methods.
  *   @history 2004-05-11 Jeff Anderson - Moved Reset, AddData and RemoveData methods into public
  *                           space.
  *   @history 2004-06-28 Jeff Anderson - Added Sum and SumSquare methods.
  *   @history 2005-02-17 Deborah Lee Soltesz - Modified file to support Doxygen documentation.
  *   @history 2005-05-23 Jeff Anderson - Changed to support 2GB+ files
  *   @history 2006-02-15 Jacob Danton - Added Valid Range options/methods
  *   @history 2006-03-10 Jacob Danton - Added Z-score method
  *   @history 2007-01-18 Robert Sucharski - Added AddData method for a single double value
  *   @history 2008-05-06 Steven Lambright - Added AboveRange, BelowRange methods
  *   @history 2010-03-18 Sharmila Prasad  - Error message more meaningful for SetValidRange function
  *   @history 2011-06-13 Ken Edmundson - Added Rms method.
  *   @history 2011-06-23 Jeannie Backer - Added QDataStream read(), write() methods and added
  *                           QDataStream >> and << operators. Replaced std strings with QStrings.
  *   @history 2014-09-05 Jeannie Backer - Added xml read/write capabilities.  Moved method
  *                           implementation to cpp file. Improved coverage of unitTest. Brought
  *                           code closer to standards.
  *
  *   @todo 2005-02-07 Deborah Lee Soltesz - add example using cube data to the class documentation
  *
  */
  class Statistics : public QObject {
    Q_OBJECT
    public:
      Statistics(QObject *parent = 0);
      Statistics(Project *project, XmlStackedHandlerReader *xmlReader, QObject *parent = 0);   
      // TODO: does xml read/write stuff need Project input???
      Statistics(const Statistics &other);
      ~Statistics();
      Statistics &operator=(const Statistics &other);

      void Reset();

      void AddData(const double *data, const unsigned int count);
      void AddData(const double data);

      void RemoveData(const double *data, const unsigned int count);
      void RemoveData(const double data);

      void SetValidRange(const double minimum = Isis::ValidMinimum, 
                         const double maximum = Isis::ValidMaximum);

      double ValidMinimum() const;
      double ValidMaximum() const;
      bool InRange(const double value);
      bool AboveRange(const double value);
      bool BelowRange(const double value);

      double Average() const;
      double StandardDeviation() const;
      double Variance() const;
      double Sum() const;
      double SumSquare() const;
      double Rms() const;

      double Minimum() const;
      double Maximum() const;
      double ChebyshevMinimum(const double percent = 99.5) const;
      double ChebyshevMaximum(const double percent = 99.5) const;
      double BestMinimum(const double percent = 99.5) const;
      double BestMaximum(const double percent = 99.5) const;
      double ZScore(const double value) const;

      BigInt TotalPixels() const;
      BigInt ValidPixels() const;
      BigInt OverRangePixels() const;
      BigInt UnderRangePixels() const;
      BigInt NullPixels() const;
      BigInt LisPixels() const;
      BigInt LrsPixels() const;
      BigInt HisPixels() const;
      BigInt HrsPixels() const;
      BigInt OutOfRangePixels() const;
      bool RemovedData() const;

      void save(QXmlStreamWriter &stream, const Project *project) const;
      // TODO: does xml stuff need project???
    
      QDataStream &write(QDataStream &stream) const;
      QDataStream &read(QDataStream &stream);

    private:
      /**
       *
       * @author 2014-07-28 Jeannie Backer
       *
       * @internal
       */
      class XmlHandler : public XmlStackedHandler {
        public:
          XmlHandler(Statistics *statistics, Project *project);
          // TODO: does xml stuff need project???
          ~XmlHandler();
   
          virtual bool startElement(const QString &namespaceURI, const QString &localName,
                                    const QString &qName, const QXmlAttributes &atts);
          virtual bool characters(const QString &ch);
          virtual bool endElement(const QString &namespaceURI, const QString &localName,
                                    const QString &qName);
   
        private:
          Q_DISABLE_COPY(XmlHandler);
   
          Statistics *m_xmlHandlerStatistics;
          Project *m_xmlHandlerProject;
          // TODO: does xml stuff need project???
          QString m_xmlHandlerCharacters;
      };

      QUuid *m_id; /**< A unique ID for this object (useful for others to reference
                        this object when saving to disk).*/
      double m_sum;              //!< Sum accumulator.
      double m_sumsum;           //!< Sum-squared accumulator.
      double m_minimum;          //!< Minimum double value encountered.
      double m_maximum;          //!< Maximum double value encountered.
      double m_validMinimum;     //!< Minimum valid pixel value
      double m_validMaximum;     //!< Maximum valid pixel value
      BigInt m_totalPixels;      //!< Count of total pixels processed.
      BigInt m_validPixels;      //!< Count of valid pixels (non-special) processed.
      BigInt m_nullPixels;       //!< Count of null pixels processed.
      BigInt m_lrsPixels;        //!< Count of low instrument saturation pixels processed.
      BigInt m_lisPixels;        //!< Count of low representation saturation pixels processed.
      BigInt m_hrsPixels;        //!< Count of high instrument saturation pixels processed.
      BigInt m_hisPixels;        //!< Count of high instrument representation pixels processed.
      BigInt m_underRangePixels; //!< Count of pixels less than the valid range
      BigInt m_overRangePixels;  //!< Count of pixels greater than the valid range
      bool   m_removedData;      /**< Indicates the RemoveData method was called which implies
                                      m_minimum and m_maximum are invalid. */
  };

  // operators to read/write Statistics to/from binary data
  QDataStream &operator<<(QDataStream &stream, const Statistics &statistics);
  QDataStream &operator>>(QDataStream &stream, Statistics &statistics);

} // end namespace isis

#endif

