# Copyright (c) 2009 Nokia Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


class ChannelTypeId:
    # Sensor server channels
    AccelerometerXYZAxisData = 0x1020507E        # sensrvaccelerometersensor.h
    AmbientLightData = 0x2000BF16                # sensrvilluminationsensor.h
    MagneticNorthData = 0x2000BEDF               # sensrvmagneticnorthsensor.h
    MagnetometerXYZAxisData = 0x2000BEE0         # sensrvmagnetometersensor.h
    OrientationData = 0x10205088                 # sensrvorientationsensor.h
    RotationData = 0x10205089                    # sensrvorientationsensor.h
    ProximityMonitor = 0x2000E585                # sensrvproximitysensor.h
    TappingData = 0x1020507F                     # sensrvtappingsensor.h
    AccelerometerDoubleTappingData = 0x10205081  # sensrvtappingsensor.h
    Undefined = 0x00000000                       # sensrvtypes.h


class PropertyId:
    # Sensor server channel properties
    AxisActive = 0x00001001             # sensrvaccelerometersensor.h
    DataRate = 0x00000002               # sensrvgeneralproperties.h
    Power = 0x01001003                  # sensrvgeneralproperties.h
    Availability = 0x00000004           # sensrvgeneralproperties.h
    MeasureRange = 0x00000005           # sensrvgeneralproperties.h
    ChannelDataFormat = 0x000000006     # sensrvgeneralproperties.h
    ScaledRange = 0x000000007           # sensrvgeneralproperties.h
    ChannelAccuracy = 0x000000008       # sensrvgeneralproperties.h
    ChannelScale = 0x000000009          # sensrvgeneralproperties.h
    ChannelUnit = 0x0000000010          # sensrvgeneralproperties.h
    DblTapThreshold = 0x00001002        # sensrvtappingsensor.h
    DblTapDuration = 0x00001003         # sensrvtappingsensor.h
    DblTapLatency = 0x00001004          # sensrvtappingsensor.h
    DblTapInterval = 0x00001005         # sensrvtappingsensor.h
    AutoCalibrationActive = 0x00001006  # sensrvmagnetometersensor.h
    CalibrationLevel = 0x00001007       # sensrvmagnetometersensor.h
    #DblTapAxisActive = AxisActive
    SensorModel = 0x0000000011          # sensrvgeneralproperties.h
    SensorConnectionType = 0x0000000012 # sensrvgeneralproperties.h
    SensorDescription = 0x0000000013    # sensrvgeneralproperties.h


class ConnectionType:
    # The connection type of sensor.
    ConnectionTypeNotDefined = 0        # Connection type is not defined
    ConnectionTypeEmbedded = 1          # Sensor is embedded in Handset
    ConnectionTypeWired = 2             # Sensors which are attached to
                                              # handset with wire
    ConnectionTypeWireless = 3          # Sensors which are attached to
                                              # handset wireless
    ConnectionTypeLicenseeBase = 8192   # start of licensee connection
                                              # type range.
    ConnectionTypeLicenseeEnd = 12287   # end of licensee connection
                                              # type range.


class PropertyType:
    # sensrvproperty.h
    UninitializedProperty = 0
    IntProperty = 1
    RealProperty = 2
    BufferProperty = 3


class ArrayIndex:
    SingleProperty = -1
    ArrayPropertyInfo = -2


class PropertyRangeUsage:
    # General properties owned and defined by the framework
    PropertyRangeNotDefined = 0,      # 0x0000
    GeneralPropertyRangeBase = 1,     # 0x0001
    GeneralPropertyRangeEnd = 4095,   # 0x0FFF

    # Channel properties defined by each sensor package. These are not unique
    # across all channels
    ChannelPropertyRangeBase = 4096,  # 0x1000
    ChannelPropertyRangeEnd = 8191,   # 0x1FFF

    # A range for licensees to define their own properties. Usage is defined
    # by the licensee.
    LicenseePropertyRangeBase = 8192,  # 0x2000
    LicenseePropertyRangeEnd = 12287   # 0x2FFF


class SetPropertySuccessIndicator:
    # sensrvtypes.h
    SetPropertyIndicationUnknown, \
    SetPropertyIndicationAvailable, \
    SetPropertyIndicationPossible, \
    SetPropertyIndicationUnavailable = range(4)


class Quantity:
    # Defines the quantity the channel is measuring.
    QuantityNotUsed = -1,         # Channel doesn't provide this information.
    QuantityNotdefined = 0,       # quantity is not defined.
    QuantityAcceleration = 10,    # Channel measures acceleration
    QuantityTapping = 11,         # Channel measures tapping events
    QuantityOrientation = 12,     # Channel measures phone orientation
    QuantityRotation = 13,        # Channel measures phone rotation
    QuantityMagnetic = 14,
    QuantityAngle = 15,
    QuantityProximity = 16,
    QuantityLicenseeBase = 8192,  # start of licensee quantity range.
    QuantityLicenseeEnd = 12287   # end of licensee quantity range.


class ContextType:

    ContextTypeNotUsed = -1,  # Channel doesn't provide this information
    ContextTypeNotDefined = 0,  # Context type is not defined

    # For sensors measuring some generic, common features of the environment
    # such as pressure or temperature of the air, sound intensity, or state of
    # the weather.
    ContextTypeAmbient = 1,

    # For sensors, which are producing information of the device itself.
    ContextTypeDevice = 2,

    # For sensors which are measuring user initiated stimulus (gesture),
    # or characteristics/properties of the user (body temperature, mass,
    # heart rate).
    ContextTypeUser = 3,
    ContextTypeLicenseeBase = 8192,  # start of licensee context type
                                           # range.
    ContextTypeLicenseeEnd = 12287   # end of licensee context type
                                           # range.


class ConnectionType:
    # The connection type of sensor.
    ConnectionTypeNotDefined = 0,    # Connection type is not defined
    ConnectionTypeEmbedded = 1,      # Sensor is embedded in Handset
    ConnectionTypeWired = 2,         # Sensors attached to handset with
                                           # wire
    ConnectionTypeWireless = 3,      # Sensors which are attached to
                                           #  handset wireless
    ConnectionTypeLicenseeBase = 8192,  # start of licensee connection
                                              # type range.
    ConnectionTypeLicenseeEnd = 12287   # end of licensee connection
                                              # type range.


class ChannelUnit:
    # represents the unit of the measured data values.
    ChannelUnitNotDefined = 0,        # Unit is not defined
    SensevChannelUnitAcceleration = 10,     # Acceleration,
                                            # meter per square second (m/s^2)
    ChannelUnitGravityConstant = 11,  # Acceleration,
                                            # gravitational constant (G)
    ChannelUnitLicenseeBase = 8192,   # start of licensee channel unit
                                            # range.
    ChannelUnitLicenseeEnd = 12287    # end of licensee channel unit
                                            # range.


class ChannelDataFormat:
    """ The format of the data is represented in a channel data structure.

        Possible values:
        ESensrvChannelDataFormatAbsolute, value of the data item represents
        actual value of the measured quantity.
        ESensrvChannelDataFormatScaled, value of the data item represents
        relative value which is scaled to between maximum and minimum value of
        the measured quantity.
        KSensrvPropIdScaledRange defines range for the data item value and
        KSensrvPropIdMeasureRange defines range for the measured quantity.
        ESensrvChannelDataFormatLicenseeBase, start of licensee channel data
        format range.
        ESensrvChannelDataFormatLicenseeEnd, end of licensee channel data
        format range.

        Scaled format example:
        Measure range for the accelerometer,
        KSensrvPropIdMeasureRange: -2g to 2g.
        KSensrvPropIdScaledRange defines following values:
        Range: Min: -127  Max: 127

        Example values for the data item and their absolute values:
        Data item: -64 = > -64/127 * 2g = -1.01g
        Data item:  32 = > 32/127 * 2g = 0.51g
        Data item: 127 = > 127/127 * 2g = 2g
    """
    ChannelDataFormatNotDefined = 0,
    ChannelDataFormatAbsolute = 1,
    ChannelDataFormatScaled = 2,
    ChannelDataFormatLicenseeBase = 8192,
    ChannelDataFormatLicenseeEnd = 12287


class ErrorSeverity:
    """ Possible values:
        ESensrvErrorSeverityMinor, some async request(s) failed. If this is not
        followed by ESensrvFatal, it means listening has been successfully
        continued.
        ESensrvErrorSeverityFatal, fatal error, server internal state regarding
        this channel might be corrupted. The channel was closed and sensor
        server session terminated.
    """
    ErrorSeverityNotDefined, \
    ErrorSeverityMinor, \
    ErrorSeverityFatal = range(3)


class ChannelChangeType:
    """The type of channel change detected.
       Possible values:
       ESensrvChannelChangeTypeNotDefined, a channel not defined
       ESensrvChannelChangeTypeRemoved, a channel was removed
       ESensrvChannelChangeTypeAdded, a new channel was added
    """
    ChannelChangeTypeNotDefined, \
    ChannelChangeTypeRemoved, \
    ChannelChangeTypeAdded = range(3)


class ConditionType:
    # sensrvchannelcondition.h
    SingleLimitCondition, \
    RangeConditionLowerLimit, \
    RangeConditionUpperLimit, \
    BinaryCondition = range(4)


class ConditionOperator:
    OperatorEquals, \
    OperatorGreaterThan, \
    OperatorGreaterThanOrEquals, \
    OperatorLessThan, \
    OperatorLessThanOrEquals, \
    OperatorBinaryAnd, \
    OperatorBinaryAll = range(7)


class ConditionSetType:
    # sensrvchannelconditionset.h
    OrConditionSet = 0
    AndConditionSet = 1


### Sensor specific definition


class AccelerometerDirection:
    # sensrvtappingsensor.h
    Xplus = 0x01
    Xminus = 0x02
    X = 0x03
    Yplus = 0x04
    Yminus = 0x08
    Y = 0x0c
    Zplus = 0x10
    Zminus = 0x20
    Z = 0x30


class ProximityState:
    # sensrvproximitysensor.h
    ProximityUndefined = 0
    ProximityIndiscernible = 1
    ProximityDisc = 2


class DeviceOrientation:
    # Possible device orientations from sensrvorientationsensor.h
    OrientationUndefined, \
    OrientationDisplayUp, \
    OrientationDisplayDown, \
    OrientationDisplayLeftUp, \
    OrientationDisplayRightUp, \
    OrientationDisplayUpwards, \
    OrientationDisplayDownwards = range(7)
    RotationUndefined = -1


class AmbientLightData:
    # sensrvilluminationsensor.h
    AmbientLightVeryDark = 0
    AmbientLightDark = 20
    AmbientLightTwilight = 40
    AmbientLightLight = 60
    AmbientLightBright = 80
    AmbientLightSunny = 100


def get_logicalname(class_object, constant_number):
    # Function for querying logical name based on number
    # returns name of constant
    for t in dir(class_object):
        if getattr(class_object, t) == constant_number:
            return t
    return None


if __name__ == '__main__':
    # self test & example
    assert get_logicalname(ChannelTypeId, 0x1020507E) == \
                           'AccelerometerXYZAxisData'
