//
//  sunModel.js
//  scripts/developer
//
//  Created by Nissim Hadar on 2017/12/27.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Sun angle is based on the NOAA model - see https://www.esrl.noaa.gov/gmd/grad/solcalc/
//
(function() {
    // Utility functions for trig. calculations
    function toRadians(angle_degs) {
        return angle_degs * (Math.PI / 180);
    }
    function toDegrees(angle_rads) {
        return angle_rads * (180.0 / Math.PI);
    }

    // Code to check if Daylight Savings is active
    Date.prototype.stdTimezoneOffset = function() {
        var fy = this.getFullYear();
        if (!Date.prototype.stdTimezoneOffset.cache.hasOwnProperty(fy)) {
            var maxOffset = new Date(fy, 0, 1).getTimezoneOffset();
            var monthsTestOrder = [6, 7, 5, 8, 4, 9, 3, 10, 2, 11, 1];

            for(var mi = 0;mi < 12; ++mi) {
                var offset = new Date(fy, monthsTestOrder[mi], 1).getTimezoneOffset();
                if (offset != maxOffset) { 
                    maxOffset = Math.max(maxOffset, offset);
                    break;
                }
            }
            Date.prototype.stdTimezoneOffset.cache[fy] = maxOffset;
        }
        return Date.prototype.stdTimezoneOffset.cache[fy];
    };

    // Cache the result for per year stdTimezoneOffset so that you don't need to recalculate it when testing multiple dates in 
    // the same year.
    Date.prototype.stdTimezoneOffset.cache = {};

    Date.prototype.isDST = function() {
        return this.getTimezoneOffset() < this.stdTimezoneOffset(); 
    };    

    // The Julian Date is the number of days (fractional) that have elapsed since Jan 1st, 4713 BC
    // See https://quasar.as.utexas.edu/BillInfo/JulianDatesG.html 
    function getJulianDay(dateTime) {
        var month = dateTime.getMonth() + 1;
        var day   = dateTime.getDate()  + 1;
        var year  = dateTime.getFullYear();
        
        if (month <=  2) {
            year  -=  1;
            month += 12;
        }

        var A = Math.floor(year / 100);
        var B = 2 - A + Math.floor(A / 4);
        return Math.floor(365.25 * (year + 4716)) + Math.floor(30.6001 * (month + 1)) + day + B - 1524.5;
    }
    
    function getMinutes(dateTime) {
        var hours   = dateTime.getHours();
        var minutes = dateTime.getMinutes();
        var seconds = dateTime.getSeconds();

        if (Date.prototype.isDST()) {
            hour -= 1;
        }

        return hours * 60 + minutes + seconds / 60.0;
    }

    function calcGeomMeanAnomalySun(t) {
        var M = 357.52911 + t * (35999.05029 - 0.0001537 * t);
        return M;		// in degrees
    }

    function calcSunEqOfCenter(t) {
        var m = calcGeomMeanAnomalySun(t);
        var mrad = toRadians(m);
        var sinm = Math.sin(mrad);
        var sin2m = Math.sin(mrad + mrad);
        var sin3m = Math.sin(mrad + mrad + mrad);
        var C = sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;
        return C;		// in degrees
    }

    function calcGeomMeanLongSun(t) {
        var L0 = 280.46646 + t * (36000.76983 + t*(0.0003032))
        while(L0 > 360.0) {
            L0 -= 360.0
        }
        while(L0 < 0.0) {
            L0 += 360.0
        }
        return L0		// in degrees
    }

    function calcSunTrueLong(t) {
        var l0 = calcGeomMeanLongSun(t);
        var c  = calcSunEqOfCenter(t);
        var O  = l0 + c;
        return O;		// in degrees
    }

    function calcSunApparentLong(t) {
        var o      = calcSunTrueLong(t);
        var omega  = 125.04 - 1934.136 * t;
        var lambda = o - 0.00569 - 0.00478 * Math.sin(toRadians(omega));
        return lambda;		// in degrees
    }

    function calcMeanObliquityOfEcliptic(t) {
        var seconds = 21.448 - t * (46.8150 + t * (0.00059 - t * (0.001813)));
        var e0      = 23.0 + (26.0 + (seconds / 60.0)) / 60.0;
        return e0;		// in degrees
    }

    function calcObliquityCorrection(t) {
        var e0    = calcMeanObliquityOfEcliptic(t);
        var omega = 125.04 - 1934.136 * t;
        var e     = e0 + 0.00256 * Math.cos(toRadians(omega));
        return e;		// in degrees
    }    

    function calcSunDeclination(t) {
        var e      = calcObliquityCorrection(t);
        var lambda = calcSunApparentLong(t);

        var sint  = Math.sin(toRadians(e)) * Math.sin(toRadians(lambda));
        var theta = toDegrees(Math.asin(sint));
        return theta;		// in degrees
    }

    function calcEccentricityEarthOrbit(t) {
        var e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
        return e;		// unitless
    }
    
    function calcEquationOfTime(t) {
        // Converts from "mean" solar day (i.e. days that are exactly 24 hours)
        // to apparent solar day (as observed)
        // This is essentially the east-west component of the analemma.
        //
        // This is caused by 2 effects:  the obliquity of the ecliptic, the eccentricity of earth's orbit
        var epsilon = calcObliquityCorrection(t);
        var l0 = calcGeomMeanLongSun(t);
        var e  = calcEccentricityEarthOrbit(t);
        var m  = calcGeomMeanAnomalySun(t);

        var y = Math.tan(toRadians(epsilon) / 2.0);
        y *= y;

        var sin2l0 = Math.sin(2.0 * toRadians(l0));
        var sinm   = Math.sin(toRadians(m));
        var cos2l0 = Math.cos(2.0 * toRadians(l0));
        var sin4l0 = Math.sin(4.0 * toRadians(l0));
        var sin2m  = Math.sin(2.0 * toRadians(m));

        var Etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0 - 
            0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;
            
        return toDegrees(Etime) * 4.0;	// in minutes of time
    }
 
    function calcSunTrueAnomaly(t) {
        var m = calcGeomMeanAnomalySun(t);
        var c = calcSunEqOfCenter(t);
        var v = m + c;
        return v;		// in degrees
    }

    function calcSunRadVector(t) {
        var v = calcSunTrueAnomaly(t);
        var e = calcEccentricityEarthOrbit(t);
        var R = (1.000001018 * (1 - e * e)) / (1 + e * Math.cos(toRadians(v)));
        return R;		// in AUs
    }

    function parseJSON(json) {
        try {
            return JSON.parse(json);
        } catch (e) {
            return undefined;
        }
    }
    
    var COMPUTATION_CYCLE = 5000;       // Run every 5 seconds
    this.preload = function(entityID) { // We don't have the entityID before the preload
        // Define user data
        var userDataProperties = {
            "userData": "{ \"latitude\": 47.0, \"longitude\": 122.0 }"
        };
        Entities.editEntity(entityID, userDataProperties);
  
        Script.setInterval(
            function() {
                // Read back user data
                var userData = Entities.getEntityProperties(entityID, 'userData').userData;
                var data = parseJSON(userData);

                var latitude_degs  = data.latitude;
                var longitude_degs = data.longitude;
                
                // These are used a lot
                var latitude  = toRadians(latitude_degs);
                var longitude = toRadians(longitude_degs);
                
                var dateTime = new Date();
                
                var julianDay         = getJulianDay(dateTime);
                var localTimeMinutes  = getMinutes(dateTime);
                var timeZone          = -dateTime.getTimezoneOffset() / 60;
                var totalTime         = julianDay + localTimeMinutes/1440.0 - timeZone / 24.0;
                
                // J2000.0 is the epoch starting Jan 1st, 2000 (noon), expressed as a Julian day
                var J2000             = 2451545.0
                
                // Number of years that have passed since J2000.0
                var julianDayModified = (J2000 - 2451545.0)/36525.0;
                
                var eqTime            = calcEquationOfTime(julianDayModified)
                var theta_rads        = toRadians(calcSunDeclination(julianDayModified));
                var solarTimeFix      = eqTime + 4.0 * longitude_degs - 60.0 * timeZone;
                var earthRadVec       = calcSunRadVector(julianDayModified);
                
                var trueSolarTime = localTimeMinutes + solarTimeFix;
                while (trueSolarTime > 1440) {
                    trueSolarTime   -= 1440;
                }
                
                var hourAngle = trueSolarTime / 4.0 - 180.0;
                if (hourAngle < -180.0) {
                    hourAngle += 360.0;
                }
                var hourAngleRadians = toRadians(hourAngle);

                var csz = Math.sin(latitude) * Math.sin(theta_rads) + 
                          Math.cos(latitude) * Math.cos(theta_rads) * Math.cos(hourAngleRadians);
                csz     = Math.min(1.0, Math.max(-1.0, csz));
                
                var zenith_degs  = toDegrees(Math.acos(csz));
                var azDenom = ( Math.cos(latitude) * Math.sin(toRadians(zenith_degs)));
                if (Math.abs(azDenom) > 0.001) {
                    azRad = (( Math.sin(latitude) * Math.cos(toRadians(zenith_degs)) ) - Math.sin(theta_rads)) / azDenom;
                    if (Math.abs(azRad) > 1.0) {
                        if (azRad < 0.0) {
                            azRad = -1.0;
                        } else {
                            azRad = 1.0;
                        }
                    }
                    var solarAzimuth_degs = 180.0 - toDegrees(Math.acos(azRad))
                    if (hourAngle > 0.0) {
                        solarAzimuth_degs = -solarAzimuth_degs;
                    }
                } else {
                    if (latitude_degs > 0.0) {
                        solarAzimuth_degs = 180.0;
                    } else { 
                        solarAzimuth_degs = 0.0;
                    }
                }
                if (solarAzimuth_degs < 0.0) {
                    solarAzimuth_degs += 360.0;
                }

                // Atmospheric Refraction correction
                var exoatmElevation = 90.0 - zenith_degs;
                if (exoatmElevation > 85.0) {
                    var refractionCorrection = 0.0;
                } else {
                    var te = Math.tan(toRadians(exoatmElevation));
                    if (exoatmElevation > 5.0) {
                        var refractionCorrection = 58.1 / te - 0.07 / (te * te * te) + 0.000086 / (te * te * te * te * te);
                    } else if (exoatmElevation > -0.575) {
                        var refractionCorrection = 
                            1735.0 + exoatmElevation  * 
                            (-518.2 + exoatmElevation * (103.4 + exoatmElevation * (-12.79 + exoatmElevation * 0.711)));
                    } else {
                        var refractionCorrection = -20.774 / te;
                    }
                    refractionCorrection = refractionCorrection / 3600.0;
                }

                var solarZenith = zenith_degs - refractionCorrection;
                var solarAltitude_degs = 90.0 - solarZenith; // aka solar elevation
               
                // Convert to XYZ
                var solarAltitude = toRadians(solarAltitude_degs);
                var solarAzimuth  = toRadians(solarAzimuth_degs);
                
                var xPos =  Math.cos(solarAltitude) * Math.sin(solarAzimuth);
                var zPos =  Math.cos(solarAltitude) * Math.cos(solarAzimuth);
                var yPos = -Math.sin(solarAltitude);

                // Compute intensity, modelling the atmosphere as a spherical shell
                // The optical air mass ratio at zenith is 1.0, and around 38.0 at the horizon
                // The ratio is limited between 1 and 38
                var EARTH_RADIUS_KM = 6371.0;
                var ATMOSPHERE_THICKNESS_KM = 9.0;
                var r = EARTH_RADIUS_KM / ATMOSPHERE_THICKNESS_KM;

                var opticalAirMassRatio = Math.sqrt(r * r * csz * csz + 2 * r + 1) - r * csz;
                opticalAirMassRatio = Math.min(38.0, Math.max(1.0, opticalAirMassRatio));
                
                Entities.editEntity(
                    entityID, { 
                        keyLight : {
                            direction: { x: xPos, y: yPos, z: zPos },
                            intensity: 1.0 / opticalAirMassRatio
                        }
                    }
                );
            },

            COMPUTATION_CYCLE
        );
    };
});