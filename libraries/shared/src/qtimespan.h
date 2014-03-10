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

#ifndef QTIMESPAN_H
#define QTIMESPAN_H

#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

//Move this to qnamespace.h when integrating
namespace Qt
{
    enum TimeSpanUnit {
        Milliseconds    = 0x0001,
        Seconds         = 0x0002,
        Minutes         = 0x0004,
        Hours           = 0x0008,
        Days            = 0x0010,
        Weeks           = 0x0020,
        Months          = 0x0040,
        Years           = 0x0080,
        DaysAndTime = Days | Hours | Minutes | Seconds,
        AllUnits = Milliseconds | DaysAndTime | Months | Years,
        NoUnit = 0
    };

    Q_DECLARE_FLAGS(TimeSpanFormat, TimeSpanUnit)
}
Q_DECLARE_OPERATORS_FOR_FLAGS(Qt::TimeSpanFormat)

//end of section to move

class QTimeSpanPrivate;

class Q_CORE_EXPORT QTimeSpan
{
public:
    QTimeSpan();
    explicit QTimeSpan(qint64 msecs);
    explicit QTimeSpan(const QDateTime& reference, qint64 msecs = 0);
    explicit QTimeSpan(const QDate& reference, quint64 msecs = 0);
    explicit QTimeSpan(const QTime& reference, quint64 msecs = 0);
    explicit QTimeSpan(const QDateTime& reference, const QTimeSpan& other);
    explicit QTimeSpan(const QDate& reference, const QTimeSpan& other);
    explicit QTimeSpan(const QTime& reference, const QTimeSpan& other);
    QTimeSpan(const QTimeSpan& other);


    ~QTimeSpan();

    // constant time units
    static const QTimeSpan Second;
    static const QTimeSpan Minute;
    static const QTimeSpan Hour;
    static const QTimeSpan Day;
    static const QTimeSpan Week;

    // status/validity of the time span
    bool isEmpty() const;
    bool isNull() const;

    // This set of functions operates on a single component of the time span.
    inline int msecsPart(Qt::TimeSpanFormat format = Qt::DaysAndTime | Qt::Milliseconds) const {return part(Qt::Milliseconds, format);}
    inline int secsPart(Qt::TimeSpanFormat format = Qt::DaysAndTime) const {return part(Qt::Seconds, format);}
    inline int minutesPart(Qt::TimeSpanFormat format = Qt::DaysAndTime) const {return part(Qt::Minutes, format);}
    inline int hoursPart(Qt::TimeSpanFormat format = Qt::DaysAndTime) const {return part(Qt::Hours, format);}
    inline int daysPart(Qt::TimeSpanFormat format = Qt::DaysAndTime) const {return part(Qt::Days, format);}
    inline int weeksPart(Qt::TimeSpanFormat format = Qt::DaysAndTime) const {return part(Qt::Weeks, format);}
    //int monthsPart(Qt::TimeSpanFormat format) const;
    //int yearsPart(Qt::TimeSpanFormat format) const;
    int part(Qt::TimeSpanUnit unit, Qt::TimeSpanFormat format = Qt::DaysAndTime) const;

    bool parts(int *msecondsPtr,
               int *secondsPtr = 0,
               int *minutesPtr = 0,
               int *hoursPtr = 0,
               int *daysPtr = 0,
               int *weeksPtr = 0,
               int *monthsPtr = 0,
               int *yearsPtr = 0,
               qreal *fractionalSmallestUnit = 0) const;

    Qt::TimeSpanUnit magnitude();

    inline void setMSecsPart(int msecs, Qt::TimeSpanFormat format = Qt::DaysAndTime) {setPart(Qt::Milliseconds, msecs, format);}
    inline void setSecsPart(int seconds, Qt::TimeSpanFormat format = Qt::DaysAndTime)  {setPart(Qt::Seconds, seconds, format);}
    inline void setMinutesPart(int minutes, Qt::TimeSpanFormat format = Qt::DaysAndTime) {setPart(Qt::Minutes, minutes, format);}
    inline void setHoursPart(int hours, Qt::TimeSpanFormat format = Qt::DaysAndTime) {setPart(Qt::Hours, hours, format);}
    inline void setDaysPart(int days, Qt::TimeSpanFormat format = Qt::DaysAndTime) {setPart(Qt::Days, days, format);}
    inline void setWeeksPart(int weeks, Qt::TimeSpanFormat format = Qt::DaysAndTime) {setPart(Qt::Weeks, weeks, format);}
    inline void setMonthsPart(int months, Qt::TimeSpanFormat format = Qt::DaysAndTime) {setPart(Qt::Months, months, format);}
    inline void setYearsPart(int years, Qt::TimeSpanFormat format = Qt::DaysAndTime) {setPart(Qt::Years, years, format);}
    void setPart(Qt::TimeSpanUnit unit, int interval, Qt::TimeSpanFormat format = Qt::DaysAndTime);

    // This set of functions operator on the entire timespan and not
    // just a single component of it.
    qint64 toMSecs() const;
    inline qreal toSecs() const {return toTimeUnit(Qt::Seconds);}
    inline qreal toMinutes() const {return toTimeUnit(Qt::Minutes);}
    inline qreal toHours() const {return toTimeUnit(Qt::Hours);}
    inline qreal toDays() const {return toTimeUnit(Qt::Days);}
    inline qreal toWeeks() const {return toTimeUnit(Qt::Weeks);}
    inline qreal toMonths() const {return toTimeUnit(Qt::Seconds);}
    inline qreal toYears() const {return toTimeUnit(Qt::Seconds);}
    qreal toTimeUnit(Qt::TimeSpanUnit unit) const;

    void setFromMSecs(qint64 msecs);
    inline void setFromSecs(qreal secs) {setFromTimeUnit(Qt::Seconds, secs);}
    inline void setFromMinutes(qreal minutes)  {setFromTimeUnit(Qt::Minutes, minutes);}
    inline void setFromHours(qreal hours) {setFromTimeUnit(Qt::Hours, hours);}
    inline void setFromDays(qreal days) {setFromTimeUnit(Qt::Days, days);}
    inline void setFromWeeks(qreal weeks) {setFromTimeUnit(Qt::Weeks, weeks);}
    void setFromMonths(qreal months);
    void setFromYears(qreal years);
    void setFromTimeUnit(Qt::TimeSpanUnit unit, qreal interval);

    // Reference date
    bool hasValidReference() const;
    QDateTime referenceDate() const;
    void setReferenceDate(const QDateTime &referenceDate);
    void moveReferenceDate(const QDateTime &referenceDate);
    void setReferencedDate(const QDateTime &referencedDate);
    void moveReferencedDate(const QDateTime &referencedDate);

    // Referenced date - referenceDate() + *this
    QDateTime referencedDate() const;

    // Pretty printing
#ifndef QT_NO_DATESTRING
    QString toString(const QString &format) const;
    QString toApproximateString(int suppresSecondUnitLimit = 3,
                                Qt::TimeSpanFormat format = Qt::Seconds | Qt::Minutes | Qt::Hours | Qt::Days | Qt::Weeks);
#endif

    // Assignment operator
    QTimeSpan &operator=(const QTimeSpan& other);

    // Comparison operators
    bool operator==(const QTimeSpan &other) const;
    inline bool operator!=(const QTimeSpan &other) const {return !(operator==(other));}
    bool operator<(const QTimeSpan &other) const;
    bool operator<=(const QTimeSpan &other) const;
    inline bool operator>(const QTimeSpan &other) const {return !(operator<=(other));}
    inline bool operator>=(const QTimeSpan &other) const {return !(operator<(other));}
    bool matchesLength(const QTimeSpan &other, bool normalize = false) const;

    // Arithmetic operators. Operators that don't change *this are declared as non-members.
    QTimeSpan &operator+=(const QTimeSpan &other);
    QTimeSpan &operator+=(qint64 msecs);
    QTimeSpan &operator-=(const QTimeSpan &other);
    QTimeSpan &operator-=(qint64 msecs);
    QTimeSpan &operator*=(qreal factor);
    QTimeSpan &operator*=(int factor);
    QTimeSpan &operator/=(qreal factor);
    QTimeSpan &operator/=(int factor);
    QTimeSpan &operator|=(const QTimeSpan &other); // Union
    QTimeSpan &operator&=(const QTimeSpan &other); // Intersection

    // Ensure the reference date is before the referenced date
    QTimeSpan normalized() const;
    void normalize();
    QTimeSpan abs() const;
    bool isNegative() const;
    bool isNormal() const {return !isNegative();}

    // Naturally ordered dates
    QDateTime startDate() const;
    QDateTime endDate() const;

    // Containment
    bool contains(const QDateTime &dateTime) const;
    bool contains(const QDate &date) const;
    bool contains(const QTime &time) const;
    bool contains(const QTimeSpan &other) const;

    bool overlaps(const QTimeSpan &other) const;
    QTimeSpan overlapped(const QTimeSpan &other) const;
    QTimeSpan united(const QTimeSpan &other) const;

    // Static construction methods
#ifndef QT_NO_DATESTRING
    static QTimeSpan fromString(const QString &string, const QString &format, const QDateTime& reference = QDateTime());
    static QTimeSpan fromString(const QString &string, const QRegExp &pattern, const QDateTime& reference,
                                Qt::TimeSpanUnit unit1,
                                Qt::TimeSpanUnit unit2 = Qt::NoUnit, Qt::TimeSpanUnit unit3 = Qt::NoUnit,
                                Qt::TimeSpanUnit unit4 = Qt::NoUnit, Qt::TimeSpanUnit unit5 = Qt::NoUnit,
                                Qt::TimeSpanUnit unit6 = Qt::NoUnit, Qt::TimeSpanUnit unit7 = Qt::NoUnit,
                                Qt::TimeSpanUnit unit8 = Qt::NoUnit);
#endif
    //static QTimeSpan fromString(const QString &string, Qt::TimeSpanFormat format);
    static QTimeSpan fromTimeUnit(Qt::TimeSpanUnit unit, qreal interval, const QDateTime& reference = QDateTime());
    /*
    static QTimeSpan fromMSecs(qint64 msecs);
    static QTimeSpan fromSecs(qreal secs) {return QTimeSpan::Second * secs;}
    static QTimeSpan fromMinutes(qreal minutes) {return QTimeSpan::Minute * minutes;}
    static QTimeSpan fromHours(qreal hours) {return QTimeSpan::Hour * hours;}
    static QTimeSpan fromDays(qreal days) {return QTimeSpan::Day * days;}
    static QTimeSpan fromWeeks(qreal weeks) {return QTimeSpan::Week * weeks;}
    */

private:
#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QTimeSpan &);
    friend Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QTimeSpan &);
#endif

    QSharedDataPointer<QTimeSpanPrivate> d;
};
Q_DECLARE_TYPEINFO(QTimeSpan, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(QTimeSpan);
Q_DECLARE_METATYPE(Qt::TimeSpanUnit);

//non-member operators
Q_CORE_EXPORT QTimeSpan operator+(const QTimeSpan &left, const QTimeSpan &right);
Q_CORE_EXPORT QTimeSpan operator-(const QTimeSpan &left, const QTimeSpan &right);
Q_CORE_EXPORT QTimeSpan operator*(const QTimeSpan &left, qreal right);//no problem
Q_CORE_EXPORT QTimeSpan operator*(const QTimeSpan &left, int right);//no problem
inline QTimeSpan operator*(qreal left, const QTimeSpan &right) {return right * left;} // works
inline QTimeSpan operator*(int left, const QTimeSpan &right) {return right * left;} // works
//Q_CORE_EXPORT QTimeSpan operator*(qreal left, const QTimeSpan &right) {return right * left;} //does not work
//Q_CORE_EXPORT QTimeSpan operator*(int left, const QTimeSpan &right) {return right * left;} //does not work
Q_CORE_EXPORT QTimeSpan operator/(const QTimeSpan &left, qreal right);
Q_CORE_EXPORT QTimeSpan operator/(const QTimeSpan &left, int right);
Q_CORE_EXPORT qreal operator/(const QTimeSpan &left, const QTimeSpan &right);
Q_CORE_EXPORT QTimeSpan operator-(const QTimeSpan &right); // Unary negation
Q_CORE_EXPORT QTimeSpan operator|(const QTimeSpan &left, const QTimeSpan &right); // Union
Q_CORE_EXPORT QTimeSpan operator&(const QTimeSpan &left, const QTimeSpan &right); // Intersection

// Operators that use QTimeSpan and other date/time classes
Q_CORE_EXPORT QTimeSpan operator-(const QDateTime &left, const QDateTime &right);
Q_CORE_EXPORT QTimeSpan operator-(const QDate &left, const QDate &right);
Q_CORE_EXPORT QTimeSpan operator-(const QTime &left, const QTime &right);
Q_CORE_EXPORT QDate operator+(const QDate &left, const QTimeSpan &right);
Q_CORE_EXPORT QDate operator-(const QDate &left, const QTimeSpan &right);
Q_CORE_EXPORT QTime operator+(const QTime &left, const QTimeSpan &right);
Q_CORE_EXPORT QTime operator-(const QTime &left, const QTimeSpan &right);
Q_CORE_EXPORT QDateTime operator+(const QDateTime &left, const QTimeSpan &right);
Q_CORE_EXPORT QDateTime operator-(const QDateTime &left, const QTimeSpan &right);


#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QTimeSpan &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QTimeSpan &);
#endif // QT_NO_DATASTREAM

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_NO_DATESTRING)
Q_CORE_EXPORT QDebug operator<<(QDebug, const QTimeSpan &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTIMESPAN_H
