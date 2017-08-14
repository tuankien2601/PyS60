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


from _sensorfw import *
from sensor_defs import *


class _Channel:
    '''Define _Channel Python class, access to low level sensor data.'''

    def __init__(self, channel_id):
        self.__channel_id = channel_id
        self.__channel = open_channel(self.__channel_id)
        self.__data_event_callback = None
        self.__data_error_callback = None
        self.__data_listening = False

    def start_data_listening(self, event_cb, error_cb):
        self.__data_event_callback = event_cb
        self.__data_error_callback = error_cb
        data_listening = \
            self.__channel.start_data_listening(self.__data_event_callback,
                                                self.__data_error_callback)
        self.__data_listening = data_listening
        return data_listening

    def stop_data_listening(self):
        not_data_listening = self.__channel.stop_data_listening()
        self.__data_listening = not not_data_listening
        return not_data_listening

    def is_data_listening(self):
        return self.__data_listening

    def channel_id(self):
        return self.__channel_id

    def properties(self):
        return self.__channel.get_all_properties()

    def set_property(self, property_id, item_index, array_index, value):
        return self.__channel.set_property(property_id, item_index,
                                           array_index, value)


class _filterBase:

    def __init__(self, window_len=10, dimensions=3):
        self.window_len = window_len
        self.dimensions = dimensions

        # initialize data buffer
        self.buffer = [0] * dimensions
        for t in range(dimensions):
            self.buffer[t] = [0] * self.window_len
        self.pointer = 0

    def update_buffer(self, *data):
        self.result = [0] * self.dimensions
        for t in range(self.dimensions):
            self.buffer[t][self.pointer] = data[t]
        self.pointer += 1
        self.pointer = self.pointer % self.window_len


class DummyFilter(_filterBase):

    def filter(self, *data):
        return data


class MedianFilter(_filterBase):

    def filter(self, *data):
        self.update_buffer(*data)
        for t in range(self.dimensions):
            tmp = self.buffer[t][:]
            tmp.sort()
            self.result[t] = tmp[self.window_len/2]
        return self.result


class LowPassFilter(_filterBase):

    def filter(self, *data):
        self.update_buffer(*data)
        for t in range(self.dimensions):
            self.result[t] = int(self.sum(self.buffer[t]) / self.window_len)
        return self.result

    def sum(self, buff):
        # default sum() function is not avaialble in s60
        return reduce(lambda x, y: x+y, buff)


def list_channels():
    result = []
    # Get all available sensor channels
    _channels = channels()

    # store basic information about each sensor channel
    for channel_id, channel_info in _channels.items():
        channel_name = get_logicalname(ChannelTypeId,
                                       channel_info['channel_type'])
        result.append({
            'id': channel_id,
            'type': channel_info['channel_type'],
            'name': channel_name})
    return result


class _Sensor:
    channel_id = None

    def __init__(self, data_filter=None):
        if data_filter is None:
            data_filter = DummyFilter()
        self.filter = data_filter
        self._channels = []
        self.timestamp = 0
        self.listening = 0
        self.channel = None
        self._counter = 0
        self.data_callback = None
        self.error_callback = None
        self._channels = list_channels()

    def set_callback(self, data_callback, error_callback=None):
        self.data_callback = data_callback
        self.error_callback = error_callback

    def custom_cb(self):
        if self.data_callback is not None:
            self.data_callback()

    def error_cb(self, error):
        ''' Error callback function'''
        if self.error_callback is not None:
            self.error_callback(error)

    def _list_channel_properties(self, channel):
        # generates printout for debuggin purposes
        channel_properties = channel.properties()
        for t in channel_properties:
            property_id, array_index, item_index = t

            if array_index < 0:
                array_index = get_logicalname(ArrayIndex, array_index)

            print get_logicalname(PropertyId, property_id), \
                  get_logicalname(PropertyType,
                                  channel_properties[t]['property_type']),\
                                  array_index, item_index
            print channel_properties[t]

    def _get_channel_property(self, channel, property):
        result = []
        channel_properties = channel.properties()
        for t in channel_properties:
            property_id, array_index, item_index = t
            if property_id == property:
                result.append([t, channel_properties[t]])
        return result

    def start_listening(self):
        if self.channel_id is not None and self.channel is None:
            # open channel if it is not yet opened
            self._openchannel(self.channel_id)

        # start listening if not yet listening
        if self.listening != 1:
            self._startlistening(self.data_cb)
            return True
        return False

    def _openchannel(self, channel_id):
        if self.channel != None:
            return False # only one channel per instance allowed now

        for t in self._channels:
            if t['type'] == channel_id:
                self.channel = _Channel(t['id'])
                return self.channel
        return False

    def _startlistening(self, cb):
        if self.channel is not None and self.listening != 1:
            self.listening = self.channel.start_data_listening(cb,
                                                               self.error_cb)
            if self.listening == 1:
                return True # success

        return False # failed

    def stop_listening(self):
        if self.channel is not None and self.listening == 1:
            self.channel.stop_data_listening()
            self.channel = None
            self.listening = 0

    def data_cb(self, data):
        # override this function
        # unpack data and store it
        # call self.custom_cb() in the end
        self.custom_cb()


class _StreamDataSensor(_Sensor):

    def get_available_data_rates(self):
        # returns available datarates as list. Number order in list
        # corresponds array_index numbers
        data_rate_properties = self._get_channel_property(self.channel,
                                                      PropertyId.DataRate)
        data_rates = [0] * (len(data_rate_properties) - 1)
        for data_rate_property in data_rate_properties:
            property_key, property_value = data_rate_property
            if property_value['property_array_index'] >= 0:
                data_rates[property_value['property_array_index']] = \
                    property_value['value']
        return data_rates

    def get_data_rate(self):
        # returns current data rate
        properties = self.channel.properties()
        data_rate_index = properties[(PropertyId.DataRate,
                                   ArrayIndex.ArrayPropertyInfo,
                                   -1)]['value']
        available_data_rates = self.get_available_data_rates()
        return available_data_rates[data_rate_index]

    def set_data_rate(self, data_rate):
        available_data_rates = self.get_available_data_rates()
        if data_rate not in available_data_rates:
            raise ValueError("Invalid data rate")
        # set property, NOTE/FIXME parameter order in wrapper is : id,
        # arrayind, itemind, value
        self.channel.set_property(PropertyId.DataRate,
                                  -1,
                                  ArrayIndex.ArrayPropertyInfo,
                                  available_data_rates.index(datarate))
        return True


class _AccelerometerSensor(_StreamDataSensor):

    def set_measure_range(self, measurerange):
        # hackish way to set range
        # 0 = +-2g
        # 1 = +-8g
        self.channel.set_property(PropertyId.MeasureRange, -1,
            ArrayIndex.ArrayPropertyInfo, measurerange)

    def get_measure_range(self):
        # hackish way to get range
        # 0 = +-2g
        # 1 = +-8g
        return self.channel.properties()[PropertyId.MeasureRange,
            ArrayIndex.ArrayPropertyInfo, -1]['value']


# FIXME : check always that channel actually exists
class AmbientLightData(_Sensor):
    channel_id = ChannelTypeId.AmbientLightData
    ambient_light = 0

    def data_cb(self, data):
        self.ambient_light = data['iAmbientLight']
        self.timestamp = data['timestamp']
        self.custom_cb()


class ProximityMonitor(_Sensor):
    channel_id = ChannelTypeId.ProximityMonitor
    proximity_state = 0

    def data_cb(self, data):
        self.proximity_state = data['iProximityState']
        self.timestamp = data['timestamp']
        self.custom_cb()


class OrientationData(_Sensor):
    channel_id = ChannelTypeId.OrientationData
    device_orientation = -1

    def data_cb(self, data):
        self.device_orientation = data['orientation']
        self.timestamp = data['timestamp']
        self.custom_cb()


class MagneticNorthData(_Sensor):
    azimuth = 0
    channel_id = ChannelTypeId.MagneticNorthData

    def data_cb(self, data):
        self.azimuth = data["yaw"]
        self.timestamp = data['timestamp']
        self.custom_cb()


class MagnetometerXYZAxisData(_Sensor):
    channel_id = ChannelTypeId.MagnetometerXYZAxisData
    x, y, z = 0, 0, 0
    calib_level = 0

    def data_cb(self, data):
        x, y, z = data["axis_x_calib"], data["axis_y_calib"], \
                  data["axis_z_calib"]
        #x,y,z = data["axis_x_raw"], data["axis_y_raw"], data["axis_z_raw"]
        self.x, self.y, self.z = self.filter.filter(x, y, z)
        properties = self.channel.properties()
        calib_property = properties[(PropertyId.CalibrationLevel,
                                     -1,
                                     ArrayIndex.SingleProperty)]
        self.calib_level = calib_property['value']
        self.timestamp = data['timestamp']
        self.custom_cb()


class AccelerometerDoubleTappingData(_AccelerometerSensor):
    channleID = ChannelTypeId.AccelerometerDoubleTappingData
    direction = 0

    def __init__(self, *args, **kwargs):
        _Sensor.__init__(self, *args, **kwargs)
        # open channel already here so it is possible to configure property
        # values before starting to listen
        self._openchannel(ChannelTypeId.AccelerometerDoubleTappingData)

    def data_cb(self, data):
        self.direction = data["direction"]
        self.timestamp = data['timestamp']
        self.custom_cb()

    # FIXME, remove this and use listen method from base class

    def start_listening(self):
        return self._startlistening(self.data_cb)

    def get_axis_active(self):
        # returns 1 if axis is active, otherwise 0.
        properties = self.channel.properties()
        x, y, z = [properties[(PropertyId.AxisActive,
                      ArrayIndex.SingleProperty, t)]['value']
                      for t in range(1, 4)]
        return x, y, z

    def get_properties(self):
        properties = self.channel.properties()

        DblTapThresholdValue = properties[(PropertyId.DblTapThreshold, -1,
            ArrayIndex.SingleProperty)]['value']
        DblTapDurationValue = properties[(PropertyId.DblTapDuration, -1,
            ArrayIndex.SingleProperty)]['value']
        DblTapLatencyValue = properties[(PropertyId.DblTapLatency, -1,
            ArrayIndex.SingleProperty)]['value']
        DblTapIntervalValue = properties[(PropertyId.DblTapInterval, -1,
            ArrayIndex.SingleProperty)]['value']

        return {"DoubleTapThreshold": DblTapThresholdValue,
                "DoubleTapDuration": DblTapDurationValue,
                "DoubleTapLatency": DblTapLatencyValue,
                "DoubleTapInterval": DblTapIntervalValue}

    def set_axis_active(self, x=None, y=None, z=None):
        if x is not None:
            self.channel.set_property(PropertyId.AxisActive, 1,
                                      ArrayIndex.SingleProperty,
                                      x)
        if y is not None:
            self.channel.set_property(PropertyId.AxisActive, 2,
                                      ArrayIndex.SingleProperty,
                                      y)
        if z is not None:
            self.channel.set_property(PropertyId.AxisActive, 3,
                                      ArrayIndex.SingleProperty,
                                      z)

    def set_properties(self,
                      DoubleTapThreshold=None,
                      DoubleTapDuration=None,
                      DoubleTapLatency=None,
                      DoubleTapInterval=None):

        if DblTapThresholdValue is not None:
            self.channel.set_property(PropertyId.DblTapThreshold,
                                      -1,
                                      ArrayIndex.SingleProperty,
                                      DoubleTapThreshold)

        if DblTapDurationValue is not None:
            self.channel.set_property(PropertyId.DblTapDuration,
                                      -1,
                                      ArrayIndex.SingleProperty,
                                      DoubleTapDuration)

        if DblTapLatencyValue is not None:
            self.channel.set_property(PropertyId.DblTapLatency,
                                      -1,
                                      ArrayIndex.SingleProperty,
                                      DoubleTapLatency)

        if DblTapIntervalValue is not None:
            self.channel.set_property(PropertyId.DblTapInterval,
                                      -1,
                                      ArrayIndex.SingleProperty,
                                      DoubleTapInterval)


class AccelerometerXYZAxisData(_AccelerometerSensor):
    channel_id = ChannelTypeId.AccelerometerXYZAxisData
    x, y, z = 0, 0, 0

    def __init__(self, *args, **kwargs):
        _Sensor.__init__(self, *args, **kwargs)
        # open channel already here so it is possible to configure property
        # values before starting to listen
        self._openchannel(ChannelTypeId.AccelerometerXYZAxisData)

    def data_cb(self, data):
        x, y, z = data["axis_x"], data["axis_y"], data["axis_z"]
        self.x, self.y, self.z = self.filter.filter(x, y, z)
        self.timestamp = data['timestamp']
        self.custom_cb()


class RotationData(_Sensor):
    channel_id = ChannelTypeId.RotationData
    x, y, z = 0, 0, 0

    def data_cb(self, data):
        x, y, z = data["axis_x"], data["axis_y"], data["axis_z"]
        self.x, self.y, self.z = self.filter.filter(x, y, z)
        self.timestamp = data['timestamp']
        self.custom_cb()
