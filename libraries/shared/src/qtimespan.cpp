/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
//#include "private/qdatetime_p.h"
#include <limits>
#include <math.h>
#include <QRegExp>
#include "qdatastream.h"
#include "qlocale.h"
#include "qtimespan.h"
#include "qdebug.h"
#include "qcoreapplication.h"
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
#include <qt_windows.h>
#endif
#ifndef Q_WS_WIN
#include <locale.h>
#endif

#include <time.h>
#if defined(Q_OS_WINCE)
#include "qfunctions_wince.h"
#endif

#if defined(Q_WS_MAC)
#include <private/qcore_mac_p.h>
#endif

#if defined(Q_OS_SYMBIAN)
#include <e32std.h>
#endif

#include "SharedLogging.h"


/*!
    \class QTimeSpan
    \brief The QTimeSpan represents a span of time
    \since 4.8

    QTimeSpan represents a span of time, which is optionally in reference to a specific
    point in time. A QTimeSpan behaves slightly different if it has a reference date or time
    or not.

    \section1 Constructing a QTimeSpan
    A QTimeSpan can be created by initializing it directly with a length and optionally with
    a reference (start) date, or by substracting two QDate or QDateTime values. By substracting
    QDate or QDateTime values, you create a QTimeSpan with the QDate or QDateTime on the right
    hand side of the - operator as the reference date.

    \code
    //Creates a QTimeSpan representing the time from October 10, 1975 to now
    QDate birthDay(1975, 10, 10);
    QTimeSpan age = QDate::currentDate() - birthDay;
    \endcode

    QTimeSpan defines a series of constants that can be used for initializing a QTimeSpan.
    Second, Minute, Hour, Day and Week are all QTimeSpan instances with the corresponding
    length and no reference date. You can use those to create new instances. See the
    section on Date arithmatic below.

    \code
    //Creates a QTimeSpan representing 2 days, 4 hours and 31 minutes.
    QTimeSpan span(2 * QTimeSpan::Day + 4 * QTimeSpan::Hour + 31 * QTimeSpan::Minute);
    \endcode

    Finally, a QTimeSpan can be constructed by using one of the static constructors
    fromString or fromTimeUnit.

    \section1 Date arithmatic
    A negative QTimeSpan means that the reference date lies before the referenced date. Call
    normalize() to ensure that the reference date is smaller or equal than the referenced date.

    Basic arithmatic can be done with QTimeSpan. QTimeSpans can be added up or substracted, or
    be multiplied by a scalar factor. For this, the usual operators are implemented. The union
    of QTimeSpans will yield the minimal QTimeSpan that covers both the original QTimeSpans,
    while the intersection will yield the overlap between them (or an empty one if there is no
    overlap). Please refer to the method documentation for details on what happens to a
    reference date when using these methods.

    QTimeSpans can also be added to or substracted from a QDate, QTime or QDateTime. This will yield
    a new QDate, QTime or QDateTime moved by the value of the QTimeSpan. Note that the QTimeSpan must
    be the right-hand argument of the operator. You can not add a QDate to a QTimeSpan, but you
    can do the reverse.

    \code
    QTimeSpan span(QTimeSpan::Hour * 5 + 45 * QTimeSpan::Minute);
    QDateTime t1 = QDateTime::currentDateTime();
    QDateTime t2 = t1 + span; // t2 is now the date time 5 hours and 45 minutes in the future.
    \endcode

    \section1 Accessing the length of a QTimeSpan
    There are two sets of methods that return the length of a QTimeSpan. The to* methods such
    as toSeconds and toMinutes return the total time in the requested unit. That may be a
    fractional number.

    \code
    QTimeSpan span = QTimeSpan::Hour * 6;
    qreal days = span.toDays(); //yields 0.25
    \endcode

    On the other hand, you may be interested in a number of units at the same time. If you want
    to know the number of days, hours and minutes in a QTimeSpan, you can use the to*Part
    methods such as toDayPart and toHourPart. These functions take a QTimeSpan::TimeSpanFormat
    argument to indicate the units you want to use for the presentation of the QTimeSpan.
    This is used to calculate the number of the requested time units. You can also use the
    parts method directly, passing pointers to ints for the units you are interested in and 0
    for the other units.

    \section1 Using months and years
    QTimeSpan can be used to describe any length of time, ranging from milliseconds to decades and
    beyond (up to the maximum value of a qint64 milliseconds; enough for any application not
    dealing with geological or astronomical time scales.) It is natural to use units like months
    and years when dealing with longer time periods, such as the age of people. The problem with
    these units is that unlike the time units for a week or shorter, the length of a month or a
    year is not fixed. It it dependent on the reference date. The time period '1 month' has a
    different meaning when we are speaking of februari or januari.

    QTimeSpan can only use the month and year time units if a valid reference date has been
    set. Without a valid reference date, month and year as time units are meaningless and their
    use will result in an assert. The largest unit of time that can be expressed without a reference
    date is a week. The time period of one month is understood to mean the period
    from a day and time one month to the same date and time in the next. If the next month does
    not have that date and time, one month will be taken to mean the period to the end of that
    month.

    \example The time from januari 2, 12 PM to februari 2, 12 PM will be understood as exactly one month.
    \example The time from januari 30, 2 PM to march 1, 00:00:00.000 will also be one month, because
    februari does not have 30 days but 28 or 29 depending on the year.
    \example The time from januari 30, 2 PM to march 30, 2 PM will be 2 months.

    The same goes for years.

    QTimeSpan stores the length of time intervals as a 64 bits integer representing milliseconds.
    That means that arithmatic with time periods set as months or years may not always yield what you
    expect. A time period set as the year describing the whole of 2007 that you multiply by two or
    add to itself, will not end up having a length of two years, but of 1 year, 11 months and 30
    days, as 2008 is one day longer than 2007. When months and years are used, they are
    converted to the exact time span they describe in reference to the reference date set for
    the QTimeSpan. With another reference date, or when negated, that time span may or may not
    describe the same number of years and months.
*/

QT_BEGIN_NAMESPACE

const QTimeSpan QTimeSpan::Second = QTimeSpan(qint64(1000));
const QTimeSpan QTimeSpan::Minute = QTimeSpan(qint64(1000 * 60));
const QTimeSpan QTimeSpan::Hour   = QTimeSpan(qint64(1000 * 60 * 60));
const QTimeSpan QTimeSpan::Day    = QTimeSpan(qint64(1000 * 60 * 60 * 24));
const QTimeSpan QTimeSpan::Week   = QTimeSpan(qint64(1000 * 60 * 60 * 24 * 7));

class QTimeSpanPrivate : public QSharedData {
public:
    qint64 interval;
    QDateTime reference;
    static const int s_daysPerMonth[12];

    void addUnit(QTimeSpan* self, Qt::TimeSpanUnit unit, qreal value)
    {
        if (unit >= Qt::Months) {
            QTimeSpan tempSpan(self->referencedDate());
            tempSpan.setFromTimeUnit(unit, value);
            interval += tempSpan.toMSecs();
        } else {
            switch (unit) {
            case Qt::Weeks:
                interval += value * 1000LL * 60LL * 60LL * 24LL * 7LL;
                break;
            case Qt::Days:
                interval += value * 1000LL * 60LL * 60LL * 24LL;
                break;
            case Qt::Hours:
                interval += value * 1000LL * 60LL * 60LL ;
                break;
            case Qt::Minutes:
                interval += value * 1000LL * 60LL;
                break;
            case Qt::Seconds:
                interval += value * 1000LL;
                break;
            case Qt::Milliseconds:
                interval += value;
                break;
            default:
                break;
            }
        }
    }

    class TimePartHash: public QHash<Qt::TimeSpanUnit, int*>
    {
    public:
        TimePartHash(Qt::TimeSpanFormat format)
        {
            for (int i(Qt::Milliseconds); i <= Qt::Years; i *= 2) {
                Qt::TimeSpanUnit u = static_cast<Qt::TimeSpanUnit>(i);
                if (format.testFlag(u)) {
                    int* newValue = new int;
                    *newValue = 0;
                    insert(u, newValue); //perhaps we can optimize this not to new each int individually?

                } else {
                    insert(u, 0);
                }
            }
        }

        ~TimePartHash()
        {
            qDeleteAll(*this);
        }

        inline bool fill(const QTimeSpan& span)
        {
            bool result = span.parts(value(Qt::Milliseconds),
                                value(Qt::Seconds),
                                value(Qt::Minutes),
                                value(Qt::Hours),
                                value(Qt::Days),
                                value(Qt::Weeks),
                                value(Qt::Months),
                                value(Qt::Years));
            return result;
        }

        inline void addUnit(const Qt::TimeSpanUnit unit)
        {
            if (value(unit) != 0)
                return;

            int* newValue = new int;
            *newValue = 0;
            insert(unit, newValue);
        }

    };

    //returns a string representation of time in a single time unit
    QString unitString(Qt::TimeSpanUnit unit, int num) const
    {
        switch (unit) {
        case::Qt::Milliseconds:
            return qApp->translate("QTimeSpanPrivate", "%n millisecond(s)", 0, num);
        case::Qt::Seconds:
            return qApp->translate("QTimeSpanPrivate", "%n second(s)", 0, num);
        case::Qt::Minutes:
            return qApp->translate("QTimeSpanPrivate", "%n minute(s)", 0, num);
        case::Qt::Hours:
            return qApp->translate("QTimeSpanPrivate", "%n hour(s)", 0, num);
        case::Qt::Days:
            return qApp->translate("QTimeSpanPrivate", "%n day(s)", 0, num);
        case::Qt::Weeks:
            return qApp->translate("QTimeSpanPrivate", "%n week(s)", 0, num);
        case::Qt::Months:
            return qApp->translate("QTimeSpanPrivate", "%n month(s)", 0, num);
        case::Qt::Years:
            return qApp->translate("QTimeSpanPrivate", "%n year(s)", 0, num);
        default:
            return QString();
        }
    }

#ifndef QT_NO_DATESTRING
    struct TimeFormatToken
    {
        Qt::TimeSpanUnit type; //Qt::NoUnit is used for string literal types
        int length;            //number of characters to use
        QString string;        //only used for string literals
    };

    QList<TimeFormatToken> parseFormatString(const QString& formatString, Qt::TimeSpanFormat &format) const
    {
        QHash<QChar, Qt::TimeSpanUnit> tokenHash;
        tokenHash.insert(QChar('y'), Qt::Years);
        tokenHash.insert(QChar('M'), Qt::Months);
        tokenHash.insert(QChar('w'), Qt::Weeks);
        tokenHash.insert(QChar('d'), Qt::Days);
        tokenHash.insert(QChar('h'), Qt::Hours);
        tokenHash.insert(QChar('m'), Qt::Minutes);
        tokenHash.insert(QChar('s'), Qt::Seconds);
        tokenHash.insert(QChar('z'), Qt::Milliseconds);


        QList<TimeFormatToken> tokenList;
        format = Qt::NoUnit;
        int pos(0);
        int length(formatString.length());
        bool inLiteral(false);
        while (pos < length) {
            const QChar currentChar(formatString[pos]);
            if (inLiteral) {
                if (currentChar == QLatin1Char('\'')) {
                    inLiteral = false; //exit literal string mode
                    if ((pos+1)<length) {
                        if (formatString[pos+1] == QLatin1Char('\'')) {
                            ++pos;

                            TimeFormatToken token = tokenList.last();
                            token.string.append(QChar('\''));
                            token.length = token.string.length();
                            tokenList[tokenList.length()-1] = token;

                            inLiteral = true; //we *are* staying in literal string mode
                        }
                    }
                } else {
                    TimeFormatToken token = tokenList.last();
                    token.string.append(currentChar);
                    token.length = token.string.length();
                    tokenList[tokenList.length()-1] = token;
                }
            } else { //not in literal string
                if (currentChar == QLatin1Char('\'')) {
                    inLiteral = true; //enter literal string mode
                    TimeFormatToken token;
                    token.type = Qt::NoUnit;
                    token.length = 0;
                    tokenList << token;
                } else {
                    if (tokenHash.contains(currentChar)) {
                        Qt::TimeSpanUnit unit = tokenHash.value(currentChar);

                        TimeFormatToken token;
                        token.length = 0;
                        token.type = unit;
                        if (!tokenList.isEmpty()) {
                            if (tokenList.last().type == unit) {
                                token = tokenList.takeLast();
                            }
                        }

                        token.length+=1;
                        tokenList.append(token);

                        format |= unit;
                    } else {
                        //ignore character?
                        TimeFormatToken token;
                        token.length =0;
                        token.type = Qt::NoUnit;
                        if (!tokenList.isEmpty()) {
                            if (tokenList.last().type == Qt::NoUnit) {
                                token = tokenList.takeLast();
                            }
                        }
                        token.string.append(currentChar);
                        token.length= token.string.length();
                        tokenList.append(token);
                    }
                }
            }

            ++pos;
        }

        return tokenList;
    }
#endif

};
const int QTimeSpanPrivate::s_daysPerMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};


/*!
    \brief Default constructor

    Constructs a null QTimeSpan
*/
QTimeSpan::QTimeSpan()
    : d(new QTimeSpanPrivate)
{
    d->interval = 0;
}

/*!
    \brief Constructor

    Constructs QTimeSpan of size msecs milliseconds. The reference date will
    be invalid.
*/
QTimeSpan::QTimeSpan(qint64 msecs)
    : d(new QTimeSpanPrivate)
{
    d->interval = msecs;
}

/*!
    \brief Copy Constructor
*/
QTimeSpan::QTimeSpan(const QTimeSpan& other):
        d(other.d)
{
}

/*!
    \brief Constructor

    Constructs QTimeSpan of size msecs milliseconds from the given reference date
    and time.
*/
QTimeSpan::QTimeSpan(const QDateTime &reference, qint64 msecs)
    : d(new QTimeSpanPrivate)
{
    d->interval = msecs;
    d->reference = reference;
}

/*!
    \brief Constructor

    Constructs QTimeSpan of size msecs milliseconds from the given reference date.
    The reference time will be 0:00:00.000
*/
QTimeSpan::QTimeSpan(const QDate &reference, quint64 msecs)
    : d(new QTimeSpanPrivate)
{
    d->interval = msecs;
    d->reference = QDateTime(reference);
}

/*!
    \brief Constructor

    Constructs QTimeSpan of size msecs milliseconds from the given reference time.
    The reference date will be today's date.
*/
QTimeSpan::QTimeSpan(const QTime &reference, quint64 msecs)
    : d(new QTimeSpanPrivate)
{
    d->interval = msecs;
    QDateTime todayReference(QDate::currentDate());
    todayReference.setTime(reference);
    d->reference = todayReference;
}

/*!
    \brief Constructor

    Constructs a QTimeSpan of the same length as other from the given reference date time.
*/
QTimeSpan::QTimeSpan(const QDateTime& reference, const QTimeSpan& other)
    : d(new QTimeSpanPrivate)
{
    d->reference = reference;
    d->interval = other.d->interval;
}

/*!
    \brief Constructor

    Constructs a QTimeSpan of the same length as other from the given reference date.
    The reference time will be 00:00:00.000
*/
QTimeSpan::QTimeSpan(const QDate& reference, const QTimeSpan& other)
    : d(new QTimeSpanPrivate)
{
    d->reference = QDateTime(reference);
    d->interval = other.d->interval;
}

/*!
    \brief Constructor

    Constructs a QTimeSpan of the same length as other from the given reference time.
    The reference date will be today's date.
*/
QTimeSpan::QTimeSpan(const QTime& reference, const QTimeSpan& other)
    : d(new QTimeSpanPrivate)
{
    QDateTime todayReference(QDate::currentDate());
    todayReference.setTime(reference);
    d->reference = todayReference;
    d->interval = other.d->interval;
}


/*!
    \brief Destructor
*/
QTimeSpan::~QTimeSpan()
{
}

/*!
    \returns true if the time span is 0; that is, if no time is spanned by
    this instance. There may or may not be a valid reference date.

    \sa isNull hasValidReference
*/
bool QTimeSpan::isEmpty() const
{
    return d->interval == 0;
}

/*!
    \returns true if the time span is 0; that is, if no time is spanned by
    this instance and there is no valid reference date.

    \sa isEmpty
*/
bool QTimeSpan::isNull() const
{
    return isEmpty() && (!hasValidReference());
}

/*!
  \brief Assignment operator
*/
QTimeSpan& QTimeSpan::operator=(const QTimeSpan& other) {
    if (&other == this)
        return *this;

    d = other.d;
    return *this;
}

/*!
    \returns a new QTimeSpan instance initialized to the indicated number of
    time units. The default reference date is invalid.

    \note that you can only construct a valid QTimeSpan using the Months or Years
    time units if you supply a valid reference date.

    \sa setFromTimeUnit
*/
QTimeSpan QTimeSpan::fromTimeUnit(Qt::TimeSpanUnit unit, qreal interval, const QDateTime& reference )
{
    switch (unit){ //note: fall through is intentional!
    case Qt::Weeks:
        interval *= 7.0;
    case Qt::Days:
        interval *= 24.0;
    case Qt::Hours:
        interval *= 60.0;
    case Qt::Minutes:
        interval *= 60.0;
    case Qt::Seconds:
        interval *= 1000.0;
    case Qt::Milliseconds:
        break;
    default:
        if (reference.isValid()) {
            QTimeSpan result(reference);
            result.setFromTimeUnit(unit, interval);
            return result;
        }
        Q_ASSERT_X(false, "static constructor", "Can not construct QTimeSpan from Month or Year TimeSpanUnit without a valid reference date.");
        return QTimeSpan();
    }

    return QTimeSpan(reference, qint64(interval));
}

/*!
    \returns the number of the requested units indicated by unit when formatted
    as format.

    \sa parts()
*/
int QTimeSpan::part(Qt::TimeSpanUnit unit, Qt::TimeSpanFormat format) const
{
    if (!format.testFlag(unit))
        return 0;

    if (!hasValidReference()) {
        if (unit == Qt::Months || unit == Qt::Years) {
            Q_ASSERT_X(false, "part", "Can not calculate Month or Year part without a reference date");
        }
        if (format.testFlag(Qt::Months) || format.testFlag(Qt::Years)) {
            qWarning() << "Unsetting Qt::Months and Qt::Years flags from format. Not supported without a reference date";
            //should this assert instead?
            format&= (Qt::AllUnits ^ (Qt::Months | Qt::Years));
        }
    }

    //build up hash with pointers to ints for the units that are set in format, and 0's for those that are not.
    QTimeSpanPrivate::TimePartHash partsHash(format);
    bool result = partsHash.fill(*this);

    if (!result) {
        //what to do? Assert perhaps?
        qWarning() << "Result is invalid!";
        return 0;
    }

    int val = *(partsHash.value(unit));
    return val;
}

#define CHECK_INT_LIMIT(interval, unitFactor) if (interval >= (qint64(unitFactor) * qint64(std::numeric_limits<int>::max()) ) ) {qWarning() << "out of range" << unitFactor; return false;}

/*!
  Retreives a breakup of the length of the QTimeSpan in different time units.

  While part() allows you to retreive the value of a single unit for a specific
  representation of time, this method allows you to retreive all these values
  with a single call. The units that you want to use in the representation of the
  time span is defined implicitly by the pointers you pass. Passing a valid pointer
  for a time unit will include that unit in the representation, while passing 0
  for that pointer will exclude it.

  The passed integer pointers will be set to the correct value so that together
  they represent the whole time span. This function will then return true.
  If it is impossible to represent the whole time span in the requested units,
  this function returns false.

  The fractionalSmallestUnit qreal pointer can optionally be passed in to
  retreive the value for the smallest time unit passed in as a fractional number.
  For instance, if your time span contains 4 minutes and 30 seconds, but the
  smallest time unit you pass in an integer pointer for is the minute unit, then
  the minute integer will be set to 4 and the fractionalSmallestUnit will be set
  to 4.5.

  A negative QTimeSpan will result in all the parts of the representation to be
  negative, while a positive QTimeSpan will result in an all positive
  representation.

  \note Months and years are only valid as units for time spans that have a valid
  reference date. Requesting the number of months or years for time spans without
  a valid reference date will return false.

  If this function returns false, the value of the passed in pointers is undefined.

  \sa part()
*/
bool QTimeSpan::parts(int *msecondsPtr,
                      int *secondsPtr,
                      int *minutesPtr,
                      int *hoursPtr,
                      int *daysPtr,
                      int *weeksPtr,
                      int *monthsPtr,
                      int *yearsPtr,
                      qreal *fractionalSmallestUnit) const
{
    /* \todo We should probably cache the results of this operation. However, that requires keeping a dirty flag
      in the private data store, or a copy of the reference date, interval and last used parts. Is that worth it?
    */

    // Has the user asked for a fractional component? If yes, find which unit it corresponds to.
    Qt::TimeSpanUnit smallestUnit = Qt::NoUnit;
    if (fractionalSmallestUnit)
    {
        if (yearsPtr)
            smallestUnit = Qt::Years;
        if (monthsPtr)
            smallestUnit = Qt::Months;
        if (weeksPtr)
            smallestUnit = Qt::Weeks;
        if (daysPtr)
            smallestUnit = Qt::Days;
        if (hoursPtr)
            smallestUnit = Qt::Hours;
        if (minutesPtr)
            smallestUnit = Qt::Minutes;
        if (secondsPtr)
            smallestUnit = Qt::Seconds;
        if (msecondsPtr)
            smallestUnit = Qt::Milliseconds;
    }

    QTimeSpan ts(*this);
    if (yearsPtr || monthsPtr) { //deal with months and years
        //we can not deal with months or years if there is no valid reference date
        if (!hasValidReference()) {
            qWarning() << "Can not request month or year parts of a QTimeSpan without a valid reference date.";
            return false;
        }

        int* _years = yearsPtr;
        if (!yearsPtr)
            _years = new int;

        *_years = 0;
        QDate startDate = ts.startDate().date();
        QDate endDate = ts.endDate().date();
        *_years = endDate.year() - startDate.year();
        if (endDate.month() < startDate.month()) {
            (*_years)--;
        } else if (endDate.month() == startDate.month()) {
            if (endDate.day() < startDate.day()) {
                (*_years)--;
            }
        }

        /** \todo Handle fractional years component */

        int* _months = monthsPtr;
        if (!monthsPtr)
            _months = new int;

        *_months = endDate.month() - startDate.month();
        if (*_months < 0)
            (*_months) += 12;
        if (endDate.day() < startDate.day()) {
            (*_months)--;
        }
        int totalMonths = (*_months);
        if (!yearsPtr)
            totalMonths += (*_years) * 12;

        QDate newStartDate(startDate);
        newStartDate = newStartDate.addYears(*_years);
        newStartDate = newStartDate.addMonths(*_months);
        //qCDebug(shared) << "working with new start date" << newStartDate << "and end date" << endDate;
        ts = QDateTime(endDate, ts.endDate().time()) - QDateTime(newStartDate, ts.startDate().time());
        *_months = totalMonths;

        /** \todo Handle fractional months component */

        //clean up temporary variables on the heap
        if (!monthsPtr)
            delete _months;
        if (!yearsPtr)
            delete _years;
    }

    //from here on, we use ts as the time span!
    qint64 intervalLeft = ts.toMSecs();
    qint64 unitFactor;
    //qCDebug(shared) << "intervalLeft" << intervalLeft;
    if (weeksPtr) {
        unitFactor = (7 * 24 * 60 * 60 * 1000);
        CHECK_INT_LIMIT(intervalLeft, unitFactor)

        *weeksPtr = intervalLeft / unitFactor;
        if (smallestUnit == Qt::Weeks)
        {
            QTimeSpan leftOverTime(referencedDate(), -intervalLeft);
            leftOverTime.normalize();
            *fractionalSmallestUnit = leftOverTime.toTimeUnit(smallestUnit);
        }

        if (*weeksPtr != 0) {
            intervalLeft = intervalLeft % unitFactor;
        }
    }

    if (daysPtr) {
        unitFactor = (24 * 60 * 60 * 1000);
        CHECK_INT_LIMIT(intervalLeft, unitFactor)

        *daysPtr = intervalLeft / unitFactor;
        if (smallestUnit == Qt::Days)
        {
            QTimeSpan leftOverTime(referencedDate(), -intervalLeft);
            leftOverTime.normalize();
            *fractionalSmallestUnit = leftOverTime.toTimeUnit(smallestUnit);
        }

        if (*daysPtr != 0 ) {
            intervalLeft = intervalLeft % unitFactor;
        }
    }

    if (hoursPtr) {
        unitFactor = (60 * 60 * 1000);
        CHECK_INT_LIMIT(intervalLeft, unitFactor)

        *hoursPtr = intervalLeft / unitFactor;
        if (smallestUnit == Qt::Hours)
        {
            QTimeSpan leftOverTime(referencedDate(), -intervalLeft);
            leftOverTime.normalize();
            *fractionalSmallestUnit = leftOverTime.toTimeUnit(smallestUnit);
        }

        if (*hoursPtr != 0 ) {
            intervalLeft = intervalLeft % unitFactor;
        }
    }

    if (minutesPtr) {
        unitFactor = (60 * 1000);
        CHECK_INT_LIMIT(intervalLeft, unitFactor)

        *minutesPtr = intervalLeft / unitFactor;
        if (smallestUnit == Qt::Minutes)
        {
            QTimeSpan leftOverTime(referencedDate(), -intervalLeft);
            leftOverTime.normalize();
            *fractionalSmallestUnit = leftOverTime.toTimeUnit(smallestUnit);
        }

        if (*minutesPtr != 0 ) {
            intervalLeft = intervalLeft % unitFactor;
        }
    }

    if (secondsPtr) {
        unitFactor = (1000);
        CHECK_INT_LIMIT(intervalLeft, unitFactor)

        *secondsPtr = intervalLeft / unitFactor;
        if (smallestUnit == Qt::Seconds)
        {
            QTimeSpan leftOverTime(referencedDate(), -intervalLeft);
            leftOverTime.normalize();
            *fractionalSmallestUnit = leftOverTime.toTimeUnit(smallestUnit);
        }

        if (*secondsPtr > 0 ) {
            intervalLeft = intervalLeft % unitFactor;
        }
    }

    if (msecondsPtr) {
        unitFactor = 1;
        CHECK_INT_LIMIT(intervalLeft, unitFactor)

        *msecondsPtr = intervalLeft;
        if (smallestUnit == Qt::Weeks)
        {
            *fractionalSmallestUnit = qreal(intervalLeft);
        }
    }

    return true;
}

/*!
  Sets a part of the time span in the given format.

  setPart allows you to adapt the current time span interval unit-by-unit based on
  any time format. Where setFromTimeUnit resets the complete time interval, setPart
  only sets a specific part in a chosen format.

  \example If you have a time span representing 3 weeks, 2 days, 4 hours, 31 minutes
  and 12 seconds, you can change the number of hours to 2 by just calling
  span.setPart(Qt::Hours, 2, Qt::Weeks | Qt::Days | Qt::Hours | Qt::Minutes | Qt::Seconds);

  Note that just like with any other function, you can not use the Months and Years
  units without using a reference date.
*/
void QTimeSpan::setPart(Qt::TimeSpanUnit unit, int interval, Qt::TimeSpanFormat format)
{
    if (!format.testFlag(unit)) {
        qWarning() << "Can not set a unit that is not part of the format. Ignoring.";
        return;
    }

    QTimeSpanPrivate::TimePartHash partsHash(format);
    bool result = partsHash.fill(*this);

    if (!result) {
        qWarning() << "Retreiving parts failed, cannot set parts. Ignoring.";
        return;
    }

    d->addUnit(this, unit, interval - *(partsHash.value(unit) ) );
}

/*!
  Returns Qt::TimeSpanUnit representing the order of magnitude of the time span.
  That is, the largest unit that can be used to display the time span that
  will result in a non-zero value.

  If the QTimeSpan does not have a valid reference date, the largest
  possible time unit that will be returned is Qt::Weeks. Otherwise,
  the largest possible time unit is Qt::Years.

  \returns Unit representing the order of magnitude of the time span.
*/
Qt::TimeSpanUnit QTimeSpan::magnitude()
{
    //abs(qint64) doesnt't compile properly with mingw32?
    qint64 mag = d->interval;
    if (mag < 0)
        mag = -mag;

    if (mag < 1000)
        return Qt::Milliseconds;
    if (mag < (1000LL * 60LL))
        return Qt::Seconds;
    if (mag < (1000LL * 60LL * 60LL))
        return Qt::Minutes;
    if (mag < (1000LL * 60LL * 60LL * 24LL))
        return Qt::Hours;
    if (mag < (1000LL * 60LL * 60LL * 24LL * 7LL))
        return Qt::Days;

    //those the simple cases. The rest is dependent on if there is a reference date
    if (hasValidReference()) {
        //simple test. If bigger than 366 (not 365!) then we are certain of dealing with years
        if (mag > (1000LL * 60LL * 60LL * 24LL * 366LL))
            return Qt::Years;
        //we need a more complicated test
        int years = 0;
        int months = 0;
        parts(0, 0, 0, 0, 0, 0, &months, &years);
        if (years > 0)
            return Qt::Years;
        if (months > 0)
            return Qt::Months;
    }

    return Qt::Weeks;
}

/*!
  \returns true if there is a valid reference date set, false otherwise.
*/
bool QTimeSpan::hasValidReference() const
{
    return d->reference.isValid();
}

/*!
  \returns the reference date. Note that the reference date may be invalid.
*/
QDateTime QTimeSpan::referenceDate() const
{
    return d->reference;
}

/*!
  Sets the reference date.

  If there currently is a reference date, the referenced date will
  not be affected. That means that the length of the time span will
  change. If there currently is no reference date set, the interval
  will not be affected and this function will have the same
  effect as moveReferenceDate.

  /sa moveReferenceDate setReferencedDate moveReferencedDate
*/
void QTimeSpan::setReferenceDate(const QDateTime &referenceDate)
{
    if (d->reference.isValid() && referenceDate.isValid()) {
        *this = referencedDate() - referenceDate;
    } else {
        d->reference = referenceDate;
    }
}

/*!
  Moves the time span to align the time spans reference date with the
  new reference date.

  Note that the length of the time span will not be modified, so the
  referenced date will shift as well. If no reference date was set
  before, it is set now and the referenced date will become valid.

  /sa setReferenceDate setReferencedDate moveReferencedDate
*/
void QTimeSpan::moveReferenceDate(const QDateTime &referenceDate)
{
    d->reference = referenceDate;
}

/*!
  Sets the referenced date.

  If there currently is a reference date, that reference date will
  not be affected. This implies that the length of the time span changes.
  If there currently is no reference date set, the interval
  will not be affected and this function will have the same
  effect as moveReferencedDate.

  /sa setReferenceDate moveReferenceDate moveReferencedDate

*/
void QTimeSpan::setReferencedDate(const QDateTime &referencedDate)
{
    if (d->reference.isValid()) {
        *this = referencedDate - d->reference;
    } else {
        d->reference = referencedDate.addMSecs(-(d->interval));
    }
}

/*!
  Moves the time span to align the time spans referenced date with the
  new referenced date.

  Note that the length of the time span will not be modified, so the
  reference date will shift as well. If no reference date was set
  before, it is set now.

  /sa setReferenceDate setReferencedDate moveReferencedDate
*/
void QTimeSpan::moveReferencedDate(const QDateTime &referencedDate)
{
    d->reference = referencedDate.addMSecs(-(d->interval));
}

/*!
  Returns the referenced date and time.

  The referenced QDateTime is the "other end" of the QTimeSpan from
  the reference date.

  An invalid QDateTime will be returned if no valid reference date
  has been set.
*/
QDateTime QTimeSpan::referencedDate() const
{
    if (!(d->reference.isValid()))
        return QDateTime();

    QDateTime dt(d->reference);
    dt = dt.addMSecs(d->interval);
    return dt;
}

// Comparison operators
/*!
  Returns true if the two compared QTimeSpans have both the same
  reference date and the same length.

  Note that two QTimeSpan objects that span the same period, but
  where one is positive and the other is negative are not considdered
  equal. If you need to compare those, compare the normalized
  versions.

  \sa matchesLength
*/
bool QTimeSpan::operator==(const QTimeSpan &other) const
{
    return ((d->interval == other.d->interval) &&
            (d->reference == other.d->reference));
}

/*!
  Returns true if the interval of this QTimeSpan is shorter than
  the interval of the other QTimeSpan.
*/
bool QTimeSpan::operator<(const QTimeSpan &other) const
{
    return d->interval < other.d->interval;
}

/*!
  Returns true if the interval of this QTimeSpan is shorter or equal
  than the interval of the other QTimeSpan.
*/
bool QTimeSpan::operator<=(const QTimeSpan &other) const
{
    return d->interval <= other.d->interval;
}

/*!
  Returns true if the interval of this QTimeSpan is equal
  to the interval of the other QTimeSpan. That is, if they have the
  same length.

  The default value of normalize is false. If normalize is true, the
  absolute values of the interval lengths are compared instead of the
  real values.

  \code
  QTimeSpan s1 =  2 * QTimeSpan::Day;
  QTimeSpan s2 = -2 * QTimeSpan::Day;
  qCDebug(shared) << s1.matchesLength(s2); //returns false
  qCDebug(shared) << s1.matchesLength(s2, true); //returns true
  \endcode
*/
bool QTimeSpan::matchesLength(const QTimeSpan &other, bool normalize) const
{
    if (!normalize) {
        return d->interval == other.d->interval;
    } else {
        return qAbs(d->interval) == qAbs(other.d->interval);
    }
}

// Arithmatic operators
/*!
  Adds the interval of the other QTimeSpan to the interval of
  this QTimeSpan. The reference date of the other QTimeSpan is
  ignored.
*/
QTimeSpan & QTimeSpan::operator+=(const QTimeSpan &other)
{
    d->interval += other.d->interval;
    return *this;
}

/*!
  Adds the number of milliseconds to the interval of
  this QTimeSpan. The reference date of the QTimeSpan is
  not affected.
*/
QTimeSpan & QTimeSpan::operator+=(qint64 msecs)
{
    d->interval += msecs;
    return *this;
}

/*!
  Substracts the interval of the other QTimeSpan from the interval of
  this QTimeSpan. The reference date of the other QTimeSpan is
  ignored while the reference date of this QTimeSpan is not affected.
*/
QTimeSpan & QTimeSpan::operator-=(const QTimeSpan &other)
{
    d->interval -= (other.d->interval);
    return *this;
}

/*!
  Substracts the number of milliseconds from the interval of
  this QTimeSpan. The reference date of the QTimeSpan is
  not affected.
*/
QTimeSpan & QTimeSpan::operator-=(qint64 msecs)
{
    d->interval -= msecs;
    return *this;
}

/*!
  Multiplies the interval described by this QTimeSpan by the
  given factor. The reference date of the QTimeSpan is not
  affected.
*/
QTimeSpan & QTimeSpan::operator*=(qreal factor)
{
    d->interval *= factor;
    return *this;
}

/*!
  Multiplies the interval described by this QTimeSpan by the
  given factor. The reference date of the QTimeSpan is not
  affected.
*/
QTimeSpan & QTimeSpan::operator*=(int factor)
{
    d->interval *= factor;
    return *this;
}

/*!
  Divides the interval described by this QTimeSpan by the
  given factor. The reference date of the QTimeSpan is not
  affected.
*/
QTimeSpan & QTimeSpan::operator/=(qreal factor)
{
    d->interval /= factor;
    return *this;
}

/*!
  Divides the interval described by this QTimeSpan by the
  given factor. The reference date of the QTimeSpan is not
  affected.
*/
QTimeSpan & QTimeSpan::operator/=(int factor)
{
    d->interval /= factor;
    return *this;
}

/*!
  \brief Modifies this QTimeSpan to be the union of two QTimeSpans.

  The union of two QTimeSpans is defined as the minimum QTimeSpan that
  encloses both QTimeSpans. The QTimeSpans need not be overlapping.

  \warning Only works if both QTimeSpans have a valid reference date.
  \sa operator&=(const QTimeSpan& other)
  \sa operator|(const QTimeSpan& other)
  \sa united(const QTimeSpan& other)
*/
QTimeSpan& QTimeSpan::operator|=(const QTimeSpan& other) // Union
{
    Q_ASSERT_X((hasValidReference() && other.hasValidReference()), "assignment-or operator", "Both participating time spans need a valid reference date");

    //do we need to check for self-assignment?

    QDateTime start = qMin(startDate(), other.startDate());
    QDateTime end = qMax(endDate(), other.endDate());

    *this = end - start;
    return *this;
}

/*!
  \brief Modifies this QTimeSpan to be the intersection of two QTimeSpans.

  The intersection of two QTimeSpans is defined as the maximum QTimeSpan that
  both QTimeSpans have in common. If the QTimeSpans don't overlap, a null QTimeSpan
  will be returned. The returned QTimeSpan will be positive.

  \warning Only works if both QTimeSpans have a valid reference date.
  \sa operator&=(const QTimeSpan& other)
  \sa operator&(const QTimeSpan& other)
  \sa overlapped(const QTimeSpan& other)
*/
QTimeSpan& QTimeSpan::operator&=(const QTimeSpan &other) // Intersection
{
    Q_ASSERT_X((hasValidReference() && other.hasValidReference()), "assignment-or operator", "Both participating time spans need a valid reference date");

    //do we need to check for self-assignment?

    const QTimeSpan* first = this;
    const QTimeSpan* last = &other;
    if (other.startDate() < startDate()) {
        first = &other;
        last = this;
    }

    //check if there is overlap at all. If not, reset the interval to 0
    if  (!(first->endDate() > last->startDate()) ) {
        d->interval = 0;
        return *this;
    }

    *this = qMin(first->endDate(), last->endDate()) - last->startDate();
    return *this;
}

/*!
  Returns true if this QTimeSpan overlaps with the other QTimeSpan.
  If one of the QTimeSpans does not have a valid reference date, this
  function returns false.
*/
bool QTimeSpan::overlaps(const QTimeSpan &other) const
{
    if (!hasValidReference() || !other.hasValidReference())
        return false;

    const QTimeSpan* first = this;
    const QTimeSpan* last = &other;
    if (other.startDate() < startDate()) {
        first = &other;
        last = this;
    }

    return (first->endDate() > last->startDate());
}

/*!
  \brief Returns a new QTimeSpan that represents the intersection of the two QTimeSpans.

  The intersection of two QTimeSpans is defined as the maximum QTimeSpan that
  both QTimeSpans have in common. If the QTimeSpans don't overlap, a null QTimeSpan
  will be returned. Any valid returned QTimeSpan will be positive.

  \warning Only works if both QTimeSpans have a valid reference date. Will assert otherwise.
  \sa operator&=(const QTimeSpan& other)
  \sa operator&(const QTimeSpan& other)

*/
QTimeSpan QTimeSpan::overlapped(const QTimeSpan &other) const
{
    Q_ASSERT_X((hasValidReference() && other.hasValidReference()), "assignment-or operator", "Both participating time spans need a valid reference date");

    const QTimeSpan* first = this;
    const QTimeSpan* last = &other;
    if (other.startDate() < startDate()) {
        first = &other;
        last = this;
    }

    //check if there is overlap at all. If not, reset the interval to 0
    if  (!(first->endDate() >= last->startDate()) ) {
        return QTimeSpan();
    }

    return qMin(first->endDate(), last->endDate()) - last->startDate();
}

/*!
  \brief Returns a new QTimeSpan that represents the union of two QTimeSpans.

  The union of two QTimeSpans is defined as the minimum QTimeSpan that
  encloses both QTimeSpans. The QTimeSpans need not be overlapping.

  \warning Only works if both QTimeSpans have a valid reference date. Will assert otherwise.
  \sa operator|=(const QTimeSpan& other)
  \sa operator|(const QTimeSpan& other)
*/
QTimeSpan QTimeSpan::united(const QTimeSpan &other) const
{
    Q_ASSERT_X((hasValidReference() && other.hasValidReference()), "assignment-or operator", "Both participating time spans need a valid reference date");

    QDateTime start = qMin(startDate(), other.startDate());
    QDateTime end = qMax(endDate(), other.endDate());

    return ( end - start );
}

/*!
  Determines if the given dateTime lies within the time span. The begin
  and end times are taken to be contained in the time span in this function.

  If the time span does not have a valid reference date, this function
  returns false.
*/
bool QTimeSpan::contains(const QDateTime &dateTime) const
{
    if (!hasValidReference())
        return false;

    return ((startDate() <= dateTime) &&
            (endDate()) >= dateTime);
}

/*!
  Determines if the given date lies within the time span. The begin
  and end times are taken to be contained in the time span in this function.
  Just like in the QTimeSpan constructors that take a QDate, the time will assumed
  to be 00:00:00.000.

  If the time span does not have a valid reference date, this function
  returns false.
*/
bool QTimeSpan::contains(const QDate &date) const
{
    QDateTime dt(date);
    return contains(dt);
}

/*!
  Determines if the given time lies within the time span. The begin
  and end times are taken to be contained in the time span in this function.
  Just like in the QTimeSpan constructors that take a QTime, the date
  will be set to today.

  If the time span does not have a valid reference date, this function
  returns false.
*/
bool QTimeSpan::contains(const QTime &time) const
{
    QDateTime dt(QDate::currentDate());
    dt.setTime(time);
    return contains(dt);
}

/*!
  Determines if the other QTimeSpan lies within this time span. Another
  time span is contained if its start time is the same or later then the
  start time of this time span, and its end time is the same or earlier
  than the end time of this time span.

  If either time span does not have a valid reference date, this function
  returns false.
*/
bool QTimeSpan::contains(const QTimeSpan &other) const
{
    if (!(hasValidReference() && other.hasValidReference()))
        return false;

    return ((startDate() <= other.startDate()) &&
            (endDate()) >= other.endDate());
}

/*!
  Returns a new QTimeSpan representing the same time span as this QTimeSpan, but
  that is guaranteed to be positive; that is, to have the reference date before or
  equal to the referenced date.

  \sa normalize abs
  */
QTimeSpan QTimeSpan::normalized() const
{
    QTimeSpan ts(*this);
    ts.normalize();
    return ts;
}

/*!
  Modifies this QTimeSpan to represent the same time span, but
  that is guaranteed to be positive; that is, to have the reference date before or
  equal to the referenced date. If there is no valid reference date, the interval
  will just be made positive.

  \sa normalized abs
  */
void QTimeSpan::normalize()
{
    if (d->interval < 0) {
        if (hasValidReference()) {
            d->reference = referencedDate();
        }
        d->interval = qAbs(d->interval);
    }
}

/*!
  Returns a copy of this QTimeSpan that is guaranteed to be positive; that is,
  to have the reference date before or equal to the referenced date. The reference
  date is not modified. That implies that if there is a reference date, and the
  QTimeSpan is negative, the returned QTimeSpan will describe the time span \i after
  the reference date instead of the time span \i before.

  If there is no valid reference date, the interval will just be made positive.

  \sa normalize normalized
  */
QTimeSpan QTimeSpan::abs() const
{
    QTimeSpan result(*this);
    result.d->interval = qAbs(result.d->interval);

    return result;
}

/*!
  Returns true if the interval is negative.
  \sa isNormal
  */
bool QTimeSpan::isNegative() const
{
    return d->interval < 0;
}

/*!
  \fn QTimeSpan::isNormal() const

  Returns true if the interval is normal, that is: not negative.
  \sa isNegative
  */

/*!
  Returns the first date of the spanned time period. If there is no valid
  reference date, an invalid QDateTime will be returned.
  */
QDateTime QTimeSpan::startDate() const
{
    if (isNegative())
        return referencedDate();

    return referenceDate();
}

/*!
  Returns the last date of the spanned time period. If there is no valid
  reference date, an invalid QDateTime will be returned.
  */
QDateTime QTimeSpan::endDate() const
{
    if (isNegative())
        return referenceDate();

    return referencedDate();
}

/*!
  Returns the duration of the QTimeSpan expressed in milliseconds. This
  value may be negative.
*/
qint64 QTimeSpan::toMSecs() const
{
    return d->interval;
}

/*!
  Returns the duration of the QTimeSpan expressed in the given TimeSpanUnit. This
  value may be negative.
*/
qreal QTimeSpan::toTimeUnit(Qt::TimeSpanUnit unit) const
{
    qreal interval = qreal(d->interval);
    switch (unit){ //fall through is intentional
    case Qt::Weeks:
        interval /= 7.0;
    case Qt::Days:
        interval /= 24.0;
    case Qt::Hours:
        interval /= 60.0;
    case Qt::Minutes:
        interval /= 60.0;
    case Qt::Seconds:
        interval /= 1000.0;
    case Qt::Milliseconds:
        break;
    default:
        Q_ASSERT_X(hasValidReference(), "toTimeUnit", "Can not convert to time units that depend on the reference date (month and year).");
        qreal result(0.0);
        int intResult(0);
        bool succes(false);
        if (unit == Qt::Months) {
            succes = parts(0, 0, 0, 0, 0, 0, &intResult, 0, &result);
        } else if (unit == Qt::Years) {
            succes = parts(0, 0, 0, 0, 0, 0, 0, &intResult, &result);
        }

        if (!succes)
            return 0.0;

        return result;
    }

    return interval;
}

/*!
  Sets the length of this QTimeSpan from the given number of milliseconds. The reference date is not
  affected.
 */
void QTimeSpan::setFromMSecs(qint64 msecs)
{
    d->interval = msecs;
}

/*!
  Sets the length of this QTimeSpan from the given number of TimeSpanUnits. The reference date is not
  affected.
 */
void QTimeSpan::setFromTimeUnit(Qt::TimeSpanUnit unit, qreal interval)
{
    switch (unit){
    case Qt::Weeks: //fall through of cases is intentional!
        interval *= 7.0;
    case Qt::Days:
        interval *= 24.0;
    case Qt::Hours:
        interval *= 60.0;
    case Qt::Minutes:
        interval *= 60.0;
    case Qt::Seconds:
        interval *= 1000.0;
    case Qt::Milliseconds:
        break;
    case Qt::Months:
        setFromMonths(interval);
        return;
    case Qt::Years:
        setFromYears(interval);
        return;
    default:
        Q_ASSERT_X(false, "setFromTimeUnit", "Can not set a QTimeSpan duration from unknown TimeSpanUnit.");
    }

    d->interval = qint64(interval);
}

/*!
  Sets the interval of the time span as a number of months.

  \warning This function can only be used if a valid reference date has been set!

  The setFromMonths method deals with fractional months in the following way: first,
  the whole number of months is extracted and added to the reference date. The fractional
  part of the number of months is then multiplied with the number of days in the month
  in which that date falls. If the number of months is negative and adding a whole month
  yields a date exactly on a month boundary, the number of days in the month before is
  used instead.
  That number is used as the number of days and is added to the interval.

  \code
  QTimeSpan ts(QDate(2010,01,01));
  ts.setFromMonths(1.5); // ts's referenced date is now februari 14, 0:00:00
  ts.setFromMonths(2.5); // ts's referenced date is now march 15, 0:00:00
  QTimeSpan ts2(QDate(2008,01,01)); //2008 is a leap year!
  ts2.setFromMonths(1.5); // ts2's referenced date is now februari 14, 12:00:00
  QTimeSpan ts3(QDate(2008,03,01)); //2008 is a leap year
  ts3.setFromMonths(-0.5); // ts3's referenced date is now februari 14: 12:00:00 //
  \endcode
  */
void QTimeSpan::setFromMonths(qreal months)
{
    Q_ASSERT_X(hasValidReference(), "setFromMonths", "Can not set interval from time unit month if there is no reference date.");

    int fullMonths = (int)months;
    qreal fractionalMonth = months - fullMonths;

    QDateTime endDate = d->reference;
    endDate = endDate.addMonths(fullMonths);

    int days = d->s_daysPerMonth[endDate.date().month()-1];
    QDateTime measureDate(endDate);
    if (fractionalMonth < 0) {
        measureDate = measureDate.addMSecs(-1);
    }
    if (QDate::isLeapYear(measureDate.date().year()) && measureDate.date().month() == 2) {
        ++days; //februari has an extra day this year...
    }

    QTimeSpan tmp = endDate - d->reference;
    qreal fractionalDays = fractionalMonth * days;
    d->interval = tmp.toMSecs() + qint64(fractionalDays * 24.0 * 60.0 * 60.0 * 1000.0);
}

/*!
  Sets the interval of the time span as a number of years.

  \warning This function can only be used if a valid reference date has been set!

  The setFromYears method deals with fractional years in the following way: first,
  the whole number of years is extracted and added to the reference date. The fractional
  part of the number of years is then multiplied with the number of days in the year
  in which that date falls. That number is used as the number of days and is added to the
  interval.
  If the number of years is negative and adding the whole years yields a date exactly on
  a year boundary, the number of days in the year before is used instead.
  */
void QTimeSpan::setFromYears(qreal years)
{
    Q_ASSERT_X(hasValidReference(), "setFromYears", "Can not set interval from time unit year if there is no reference date.");

    int fullYears = (int)years;
    qreal fractionalYear = years - fullYears;

    QDateTime endDate = d->reference;
    endDate = endDate.addYears(fullYears);

    qreal days = 365.0;
    QDateTime measureDate(endDate);
    if (fractionalYear < 0) {
        measureDate = measureDate.addMSecs(-1);
    }
    if (QDate::isLeapYear(measureDate.date().year())) {
        days += 1.0; //februari has an extra day this year...
    }

    QTimeSpan tmp = endDate - d->reference;
    qreal fractionalDays = fractionalYear * days;
    d->interval = tmp.toMSecs() + qint64(fractionalDays * 24.0 * 60.0 * 60.0 * 1000.0);
}

#ifndef QT_NO_DATASTREAM
/*!
  Streaming operator.

  This operator allows you to stream a QTimeSpan into a QDataStream.
  /sa operator>>(QDataStream &stream, QTimeSpan &span)
  */
QDataStream & operator<<(QDataStream &stream, const QTimeSpan & span)
{
    stream << span.d->reference << span.d->interval;
    return stream;
}

/*!
  Streaming operator.

  This operator allows you to stream a QTimeSpan out of a QDataStream.
  /sa operator>>(QDataStream &stream, QTimeSpan &span)
  */
QDataStream & operator>>(QDataStream &stream, QTimeSpan &span)
{
    stream >> span.d->reference >> span.d->interval;
    return stream;
}
#endif

/*!
  Adds another QTimeSpan to this QTimeSpan.

  The values of the intervals of the QTimeSpans are added up with normal
  arithmatic. Negative values will work as expected.

  If the left argument has a reference date, that reference will be kept.
  If only the right argument has a reference date, then that reference
  date will be used as the new reference date.

  The above can have suprising consequences:
  \code
  // s1 and s2 are two QTimeSpan objects
  QTimeSpan s12 = s1 + s2;
  QTimeSpan s21 = s2 + s1;

  if (s12 == s21) {
    //may or may not happen, depending on the reference dates of s1 and s2.
  }
  \endcode
*/
QTimeSpan operator+(const QTimeSpan &left, const QTimeSpan &right)
{
    QTimeSpan result(left);
    result += right;

    // only keep the right reference date if the left argument does not have one
    if (!left.hasValidReference() && right.hasValidReference()) {
        result.setReferenceDate(right.referenceDate());
    }

    return result;
}

/*!
  Substracts another QTimeSpan from this QTimeSpan.

  The value of the interval of the right QTimeSpan is substracted from the
  left QTimeSpan with normal arithmatic. Negative values will work as expected.

  If the left argument has a reference date, that reference will be kept.
  If only the right argument has a reference date, then that reference
  date will be used as the new reference date.

  \sa operator+
*/
QTimeSpan operator-(const QTimeSpan &left, const QTimeSpan &right)
{
    QTimeSpan result(left);
    result -= right;

    // only keep the right reference date if the left argument does not have one
    if (!left.hasValidReference() && right.hasValidReference()) {
        result.setReferenceDate(right.referenceDate());
    }

    return result;

}

/*!
  Multiply a QTimeSpan by a scalar factor.

  \returns a new QTimeSpan object that has the same reference date as the
  left QTimeSpan, but with an interval length that is multiplied by the
  right argument.
*/
QTimeSpan operator*(const QTimeSpan &left, qreal right)
{
    QTimeSpan result(left);
    result*=right;
    return result;
}

/*!
  Multiply a QTimeSpan by a scalar factor.

  \returns a new QTimeSpan object that has the same reference date as the
  left QTimeSpan, but with an interval length that is multiplied by the
  right argument.
*/
QTimeSpan operator*(const QTimeSpan &left, int right)
{
    QTimeSpan result(left);
    result*=right;
    return result;
}

/*!
  Devide a QTimeSpan by a scalar factor.

  \returns a new QTimeSpan object that has the same reference date as the
  left QTimeSpan, but with an interval length that is devided by the
  right argument.
*/
QTimeSpan operator/(const QTimeSpan &left, qreal right)
{
    QTimeSpan result(left);
    result/=right;
    return result;
}

/*!
  Devide a QTimeSpan by a scalar factor.

  \returns a new QTimeSpan object that has the same reference date as the
  left QTimeSpan, but with an interval length that is devided by the
  right argument.
*/
QTimeSpan operator/(const QTimeSpan &left, int right)
{
    QTimeSpan result(left);
    result/=right;
    return result;
}

/*!
  Devides two QTimeSpans. The devision works on the interval lengths of
  the two QTimeSpan objects as you would expect from normal artithmatic.
*/
qreal operator/(const QTimeSpan &left, const QTimeSpan &right)
{
    return (qreal(left.toMSecs()) / qreal(right.toMSecs()));
}

/*!
  Returns a QTimeSpan object with the same reference date as the right
  hand argument, but with a negated interval. Note that unlike with the
  normalize() method, this function will result in a QTimeSpan that
  describes a different period if the QTimeSpan has a reference date because
  the reference date is not modified.
*/
QTimeSpan operator-(const QTimeSpan &right) // Unary negation
{
    QTimeSpan result(right);
    result.setFromMSecs(-result.toMSecs());
    return result;
}

/*!
  \returns the union of the two QTimeSpans.

  \sa united
*/
QTimeSpan operator|(const QTimeSpan &left, const QTimeSpan &right)  // Union
{
    QTimeSpan result(left);
    result|=right;
    return result;
}

/*!
  \returns the intersection of the two QTimeSpans.

  \sa intersected
*/
QTimeSpan operator&(const QTimeSpan &left, const QTimeSpan &right)  // Intersection
{
    QTimeSpan result(left);
    result&=right;
    return result;
}

// Operators that use QTimeSpan and other date/time classes
/*!
  Creates a new QTimeSpan object that describes the period between the two
  QDateTime objects. The right hand object will be used as the reference date,
  so that substracting a date in the past from a date representing now will yield
  a positive QTimeSpan.

  \note that while substracting two dates will result in a QTimeSpan describing
  the time between those dates, there is no pendant operation for adding two dates.

  Substractions involving an invalid QDateTime, will result in a time span with
  an interval length 0. If the right-hand QDateTime is valid, it will still be
  used as the reference date.
*/
QTimeSpan operator-(const QDateTime &left, const QDateTime &right)
{
    QTimeSpan result(right);
#if QT_VERSION >= 0x040700
    if (left.isValid() && right.isValid()) {
        result = QTimeSpan(right, right.msecsTo(left));
    }
#else
    if (left.isValid() && right.isValid()) {
        QDateTime tmpLeft = left.toUTC();
        QDateTime tmpRight = right.toUTC();
        //don't use QDateTime::secsTo, you end up in rounding issues as well as overflow problems!
        qint64 interval = 1000LL * 60LL * 60LL * 24LL * qint64(tmpRight.date().daysTo(tmpLeft.date()));
        interval += tmpRight.time().msecsTo(tmpLeft.time());
        result.setFromMSecs(interval);
    }
#endif
    return result;
}

/*!
  Creates a new QTimeSpan object that describes the period between the two
  QDate objects. The right hand object will be used as the reference date,
  so that substracting a date in the past from a date representing now will yield
  a positive QTimeSpan.

  \note that while substracting two dates will result in a QTimeSpan describing
  the time between those dates, there is no pendant operation for adding two dates.
*/
QTimeSpan operator-(const QDate &left, const QDate &right)
{
    QTimeSpan result = QDateTime(left) - QDateTime(right);
    return result;
}

/*!
  Creates a new QTimeSpan object that describes the period between the two
  QTime objects. The right hand time will be used as the reference time,
  so that substracting a time in the past from a time representing now will yield
  a positive QTimeSpan.

  \note that that both times will be assumed to be on the current date.

  \note that while substracting two times will result in a QTimeSpan describing
  the time between those, there is no pendant operation for adding two times.
*/
QTimeSpan operator-(const QTime &left, const QTime &right)
{
    return QDateTime(QDate::currentDate(), left) - QDateTime(QDate::currentDate(), right);
}

/*!
  \returns the date described by the left-hand date, shifted by the interval
  described in the QTimeSpan. The reference date of the QTimeSpan, if set, is
  ignored.

  No rounding takes place. If a QTimeSpan describes 1 day, 23 hours and 59 minutes,
  adding that QTimeSpan to a QDate respresenting April 1 will still yield April 2.
*/
QDate operator+(const QDate &left, const QTimeSpan &right)
{
    QDateTime dt(left);
    return (dt + right).date();
}

/*!
  \returns the date and time described by the left-hand QDateTime, shifted by
  the interval described in the QTimeSpan. The reference date of the QTimeSpan, if set,
  is ignored.
*/
QDateTime operator+(const QDateTime &left, const QTimeSpan &right)
{
    QDateTime result(left);
    result = result.addMSecs(right.toMSecs());
    return result;
}

/*!
  \returns the time described by the left-hand QTime, shifted by
  the interval described in the QTimeSpan. The reference date of the QTimeSpan, if set,
  is ignored.

  \note that since QTimeSpan works with dates and times, the time returned will never
  be bigger than 23:59:59.999. The time will wrap to the next date. Use QDateTime objects
  if you need to keep track of that.
*/
QTime operator+(const QTime &left, const QTimeSpan &right)
{
    QDateTime dt(QDate::currentDate(), left);
    dt = dt.addMSecs(right.toMSecs());
    return dt.time();
}

/*!
  \returns the date described by the left-hand date, shifted by the negated interval
  described in the QTimeSpan. The reference date of the QTimeSpan, if set, is
  ignored.

  No rounding takes place. If a QTimeSpan describes 1 day, 23 hours and 59 minutes,
  adding that QTimeSpan to a QDate respresenting April 1 will still yield April 2.
*/
QDate operator-(const QDate &left, const QTimeSpan &right)
{
    QDateTime dt(left);
    return (dt - right).date();
}

/*!
  \returns the date and time described by the left-hand QDateTime, shifted by
  the negated interval described in the QTimeSpan. The reference date of the
  QTimeSpan, if set, is ignored.
*/
QDateTime operator-(const QDateTime &left, const QTimeSpan &right)
{
    QDateTime result(left);
    result = result.addMSecs( -(right.toMSecs()) );
    return result;
}

/*!
  \returns the time described by the left-hand QTime, shifted by
  the negated interval described in the QTimeSpan. The reference date of
  the QTimeSpan, if set, is ignored.

  \note that since QTimeSpan works with dates and times, the time returned will never
  be bigger than 23:59:59.999. The time will wrap to the next date. Use QDateTimes
  if you need to keep track of that.
*/
QTime operator-(const QTime &left, const QTimeSpan &right)
{
    QDateTime dt(QDate::currentDate(), left);
    dt = dt.addMSecs( -(right.toMSecs()) );
    return dt.time();
}

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_NO_DATESTRING)
/*!
  Operator to stream QTimeSpan objects to a debug stream.
*/
QDebug operator<<(QDebug debug, const QTimeSpan &ts)
{
    debug << "QTimeSpan(Reference Date =" << ts.referenceDate()
          << "msecs =" << ts.toMSecs() << ")";
    return debug;
}
#endif

//String conversions
#ifndef QT_NO_DATESTRING
/*!
  \returns an approximate representation of the time span length

  When representing the lenght of a time span, it is often not nessecairy to be
  completely accurate. For instance, when dispaying the age of a person, it is
  often enough to just state the number of years, or possibly the number of years
  and the number of months. Similary, when displaying how long a certain operation
  the user of your application started will run, it is useless to display the
  number of seconds left if the operation will run for hours more.

  toApproximateString() provides functionality to display the length of the
  QTimeSpan in such an approximate way. It will format the time using one or two
  neighbouring time units. The first time unit that will be used is the unit
  that represents the biggest portion of time in the span. The second time unit
  will be the time unit directly under that. The second unit will only be used
  if it is not 0, and if the first number is smaller than the indicated
  suppresSecondUnitLimit.

  The suppressSecondUnitLimit argument can be used to suppres, for instance,
  the number of seconds when the operation will run for more than five minutes
  more. The idea is that for an approximate representation of the time length,
  it is no longer relevant to display the second unit if the first respresents
  a time span that is perhaps an order of magnitude larger already.

  If you set suppressSecondUnitLimit to a negative number, the second unit will
  always be displayed, unless no valid unit for it could be found.
*/
QString QTimeSpan::toApproximateString(int suppresSecondUnitLimit, Qt::TimeSpanFormat format)
{
    if (format==Qt::NoUnit)
        return QString();

    //retreive the time unit to use as the primairy unit
    int primairy = -1;
    int secondairy = -1;

    Qt::TimeSpanUnit primairyUnit = magnitude();
    while (!format.testFlag(primairyUnit ) && primairyUnit > Qt::NoUnit) {
        primairyUnit = Qt::TimeSpanUnit(primairyUnit / 2);
    }

    Qt::TimeSpanUnit secondairyUnit = Qt::NoUnit;
    if (primairyUnit > 1) {
        secondairyUnit = Qt::TimeSpanUnit(primairyUnit / 2);
    } else {
        primairy = 0;
    }
    while (!format.testFlag(secondairyUnit) && secondairyUnit > Qt::NoUnit) {
        secondairyUnit = Qt::TimeSpanUnit(secondairyUnit / 2);
    }

    //build up hash with pointers to ints for the units that are set in format, and 0's for those that are not.
    if (primairy < 0) {
        QTimeSpanPrivate::TimePartHash partsHash(format);
        bool result = partsHash.fill(*this);

        if (!result) {
            qCDebug(shared) << "false result from parts function";
            return QString();
        }

        primairy = *(partsHash.value(primairyUnit));
        if (secondairyUnit > 0) {
            secondairy = *(partsHash.value(secondairyUnit));
        } else {
            secondairy = 0;
        }
    }

    if ((primairy > 0
         && secondairy > 0
         && primairy < suppresSecondUnitLimit)
        || (suppresSecondUnitLimit < 0
         && secondairyUnit > Qt::NoUnit) )
    {
        //we will display with two units
        return d->unitString(primairyUnit, primairy) + QLatin1String(", ") + d->unitString(secondairyUnit, secondairy);
    }

    //we will display with only the primairy unit
    return d->unitString(primairyUnit, primairy);
}

/*!
  \returns a string representation of the duration of this time span in the requested format

  This function returns a representation of only the length of this time span. If
  you need the reference or referenced dates, access those using one of the provided
  methods and output them directly.

  The format parameter determines the format of the result string. The duration will be
  expressed in the units you use in the format.

  y The number of years
  M The number of months
  w The number of weeks
  d The number of days
  h The number of hours
  m The number of minutes
  s The number of seconds
  z The number of milliseconds

  Use multiple letters to force leading zeros.

  \note You can not use years or months if the time span does not have a valid reference
  date.

  Characters in the string that don't represent a time unit, are used as literal strings in the
  output. Everything between single quotes will always be used as a literal string. This makes
  it possible to use the characters used for the time span format also as literal output. To use a
  single quote in the output, put two consecutive single quotes within a single quote literal
  string block. To just put a single quote in a the output, you need four consequtive single
  quotes.
*/
QString QTimeSpan::toString(const QString &format) const
{
    Qt::TimeSpanFormat tsFormat = Qt::NoUnit;
    QList<QTimeSpanPrivate::TimeFormatToken> tokenList = d->parseFormatString(format, tsFormat);

    QTimeSpanPrivate::TimePartHash partsHash(tsFormat);
    bool result = partsHash.fill(*this);

    if (!result)
        return QString();

    QString formattedString;
    foreach(QTimeSpanPrivate::TimeFormatToken token, tokenList) {
        if (token.type == 0) {
            formattedString.append(token.string);
        } else {
            Qt::TimeSpanUnit unit(token.type);
            formattedString.append (QString("%1").arg(*partsHash.value( unit ),
                                                      token.length,
                                                      10,
                                                      QChar('0') ) );
        }
    }

    return formattedString;
}

/*!
  Returns a time span represented by the string using the format given, or an empty
  time span if the string cannot be parsed.

  The optional reference argument will be used as the reference date for the string.

  \note You can only use months or years if you also pass a valid reference.
  */
QTimeSpan QTimeSpan::fromString(const QString &string, const QString &format, const QDateTime &reference)
{
    /*

     There are two possible ways of parsing a string. On the one hand, you could use the
     lengths of string literals to determine the positions in the string where you expect
     the different parts of the string. On the other hand, you could use the actual contents
     of the literals as delimiters to figure out what parts of the string refer to what
     unit of time. In that case, the length of the time units would only matter if they are
     not surrounded by a string literal. Both seem useful. Perhaps we need two different
     modes for this?

     The code here implements the first option. The overloaded version below implements a
     more flexible regexp based approach.
     */

    //stage one: parse the format string
    QTimeSpan span(reference);
    Qt::TimeSpanFormat tsFormat = Qt::NoUnit;
    QList<QTimeSpanPrivate::TimeFormatToken> tokenList = span.d->parseFormatString(format, tsFormat);

    //prepare the temporaries
    QTimeSpanPrivate::TimePartHash partsHash(tsFormat);
    QString input(string);

    //extract the values from the input string into our temporary structure
    foreach(const QTimeSpanPrivate::TimeFormatToken token, tokenList) {
        if (token.type == Qt::NoUnit) {
            input = input.remove(0, token.length);
        } else {
            QString part = input.left(token.length);
            input = input.remove(0, token.length);

            bool success(false);
            part = part.trimmed();
            int value = part.toInt(&success, 10);
            if (!success) {
                return QTimeSpan();
            }
            *(partsHash.value(token.type)) = value;
        }
    }

    //construct the time span from the temporary data
    //we must set the number of years and months first; for the rest order is not important
    if (partsHash.value(Qt::Years)) {
        span.d->addUnit(&span, Qt::Years, *(partsHash.value(Qt::Years)));
        delete partsHash.value(Qt::Years);
        partsHash.insert(Qt::Years, 0);
    }
    if (partsHash.value(Qt::Months)) {
        span.d->addUnit(&span, Qt::Months, *(partsHash.value(Qt::Months)));
        delete partsHash.value(Qt::Months);
        partsHash.insert(Qt::Months, 0);
    }

    //add the rest of the units
    QHashIterator<Qt::TimeSpanUnit, int*> it(partsHash);
    while (it.hasNext()) {
        it.next();
        if (it.value()) {
            span.d->addUnit(&span, it.key(), *(it.value()));
            qCDebug(shared) << "Added unit" << it.key() << "with value" << *(it.value()) << "new value" << span.d->interval;
        }
    }

    return span;
}

/*!
  Returns a time span represented by the string using the patern given, or an empty
  time span if the string cannot be parsed. Each pair of capturing parenthesis can
  extract a time unit. The order in which the units appear is given by the list of
  arguments unit1 to unit8. Captures for which the corresponding type is set to
  Qt::NoUnit will be ignored.

  The reference argument will be used as the reference date for the string.

  \note You can only use months or years if you also pass a valid reference.
  */
QTimeSpan QTimeSpan::fromString(const QString &string, const QRegExp &pattern, const QDateTime &reference,
                            Qt::TimeSpanUnit unit1, Qt::TimeSpanUnit unit2, Qt::TimeSpanUnit unit3,
                            Qt::TimeSpanUnit unit4, Qt::TimeSpanUnit unit5, Qt::TimeSpanUnit unit6,
                            Qt::TimeSpanUnit unit7, Qt::TimeSpanUnit unit8)
{
    if (pattern.indexIn(string) < 0)
        return QTimeSpan();

    QTimeSpanPrivate::TimePartHash partsHash(Qt::NoUnit);

    QList<Qt::TimeSpanUnit> unitList;
    unitList << unit1 << unit2 << unit3 << unit4 << unit5 << unit6 << unit7 << unit8;

    for (int i(0); i < qMin(pattern.captureCount(), 8 ); ++i) {
        if (unitList.at(i) > Qt::NoUnit) {
            partsHash.addUnit(unitList.at(i));
            QString capture = pattern.cap(i + 1);
            bool ok(false);
            int value = capture.toInt(&ok, 10);
            if (!ok)
                return QTimeSpan();

            *(partsHash.value(unitList.at(i))) = value;
        }
    }

    //create the time span to return
    QTimeSpan span(reference);

    //construct the time span from the temporary data
    //we must set the number of years and months first; for the rest order is not important
    if (partsHash.value(Qt::Years)) {
        span.d->addUnit(&span, Qt::Years, *(partsHash.value(Qt::Years)));
        delete partsHash.value(Qt::Years);
        partsHash.insert(Qt::Years, 0);
    }
    if (partsHash.value(Qt::Months)) {
        span.d->addUnit(&span, Qt::Months, *(partsHash.value(Qt::Months)));
        delete partsHash.value(Qt::Months);
        partsHash.insert(Qt::Months, 0);
    }

    //add the rest of the units
    QHashIterator<Qt::TimeSpanUnit, int*> it(partsHash);
    while (it.hasNext()) {
        it.next();
        if (it.value()) {
            span.d->addUnit(&span, it.key(), *(it.value()));
        }
    }

    return span;
}
#endif

QT_END_NAMESPACE
