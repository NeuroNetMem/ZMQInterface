/*
 ------------------------------------------------------------------
 
 This file is part of the Open Ephys GUI
 Copyright (C) 2015 Open Ephys
 
 ------------------------------------------------------------------
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */
/*
  ==============================================================================

    ZmqInterface.cpp
    Created: 19 Sep 2015 9:47:12pm
    Author:  Francesco Battaglia

  ==============================================================================
*/

#include <zmq.h>
#include <string.h>
#include <iostream>

#include "ZmqInterface.h"

ZmqInterface::ZmqInterface(const String &processorName)
    : GenericProcessor(processorName)
{
    createContext();
}

ZmqInterface::~ZmqInterface()
{
    close();
    zmq_ctx_destroy(context);
}

int ZmqInterface::createContext()
{
    context = zmq_ctx_new();
    if(!context)
        return -1;
    return 0;
}

int ZmqInterface::createDataSocket()
{
    if(!socket)
    {
        socket = zmq_socket(context, ZMQ_PUB);
        if(!socket)
            return -1;
        int rc = zmq_bind(socket, "tcp://*:5556");
        assert(rc == 0);
        rc = zmq_bind(socket, "ipc://data.ipc");
        assert(rc == 0);
        
    }
    return 0;
}

int ZmqInterface::close()
{
    if(socket)
    {
        int rc = zmq_close(socket);
        assert(rc==0);
    }
    return 0;
}

/* format for passing data
 JSON
 { "messageNo": number,
 "type": "data"|"event"|"parameter",
 "content":
 (for data)
 { "nChannels": nChannels,
 "nSamples": nSamples
 }
 (for event)
 {
 "eventType": number,
 "sampleNum": sampleNum (number),
 "eventId": id (number),
 "eventChannel": channel (number),
 }
 (for parameter)
 {
 "param_name1": param_value1,
 "param_name2": param_value2,
 ...
 }
 "dataSize": size (if size > 0 it's the size of binary data coming in in the next frame (multi-part message)
 }
 
 and then a possible data packet
 */




int ZmqInterface::sendData(float *data, int nChannels, int nSamples, int nRealSamples)
{
    
    messageNumber++;
    
//    MemoryOutputStream jsonHeader;
//    jsonHeader << "{ \"messageNo\": " << messageNumber << "," << newLine;
//    jsonHeader << "  \"type\": \"data\"," << newLine;
//    jsonHeader << " \"content\": " << newLine;
//    jsonHeader << "{ \"nChannels\": " << nChannels << "," << newLine;
//    jsonHeader << " \"nSamples\": " << nSamples << "," << newLine;
//    jsonHeader << " \"nRealSamples\": " << nRealSamples <<  newLine;
//    jsonHeader << "}," << newLine;
//    jsonHeader << " \"dataSize\": " << (int)(nChannels * nSamples * sizeof(float)) << newLine;
//    jsonHeader << "}";
//    
//    MemoryBlock headerBlock = jsonHeader.getMemoryBlock();

    DynamicObject::Ptr obj = new DynamicObject();
    
    int mn = messageNumber;
    obj->setProperty("messageNo", mn);
    obj->setProperty("type", "data");
    
    DynamicObject::Ptr c_obj = new DynamicObject();
    
    c_obj->setProperty("nChannels", nChannels);
    c_obj->setProperty("nSamples", nSamples);
    c_obj->setProperty("nRealSamples", nRealSamples);
    
    obj->setProperty("content", var(c_obj));
    obj->setProperty("dataSize", (int)(nChannels * nSamples * sizeof(float)));
    
    var json(obj);
    
    String s = JSON::toString(json);
    void *headerData = (void *)s.toRawUTF8();
    
//    std::cout << "the string: " << s << std::endl;
//    std::cout << "length: " << s.length() << std::endl;
//    std::cout << (char *)headerData << std::endl;
    size_t headerSize = s.length();
    
    zmq_msg_t messageHeader;
    zmq_msg_init_size(&messageHeader, headerSize);
    memcpy(zmq_msg_data(&messageHeader), headerData, headerSize);
    int size = zmq_msg_send(&messageHeader, socket, ZMQ_SNDMORE);
    jassert(size != -1);
    zmq_msg_close(&messageHeader);
    // std::cout << "size: " << size << std::endl;
    
    zmq_msg_t message;
    zmq_msg_init_size(&message, sizeof(float)*nSamples*nChannels);
    memcpy(zmq_msg_data(&message), data, sizeof(float)*nSamples*nChannels);
    int size_m = zmq_msg_send(&message, socket, 0);
    jassert(size_m);
    size += size_m;
    zmq_msg_close(&message);
 
    return size;
}

/*
 uint8 type,
 int sampleNum,
 uint8 eventId,
 uint8 eventChannel,
 uint8 numBytes,
 uint8* eventData,
 bool isTimestamp
 */
int ZmqInterface::sendEvent( uint8 type,
                             int sampleNum,
                             uint8 eventId,
                             uint8 eventChannel,
                             uint8 numBytes,
                             uint8* eventData)
{
    int size;
    
    messageNumber++;

//    MemoryOutputStream jsonHeader;
//    jsonHeader << "{ \"messageNo\": " << messageNumber << "," << newLine;
//    jsonHeader << "  \"type\": \"event\"," << newLine;
//    jsonHeader << " \"content\": " << newLine;
//    jsonHeader << " { \"type\": " << type << "," << newLine;
//    jsonHeader << " \"sampleNum\": " << sampleNum << "," << newLine;
//    jsonHeader << " \"eventId\": " << eventId << "," << newLine;
//    jsonHeader << " \"eventChannel\": " << eventChannel  << newLine;
//    jsonHeader << "}," << newLine;
//    jsonHeader << " \"dataSize\": " << numBytes << newLine;
//    jsonHeader << "}";
//    MemoryBlock headerBlock = jsonHeader.getMemoryBlock();
    
    DynamicObject::Ptr obj = new DynamicObject();
    
    obj->setProperty("messageNo", messageNumber);
    obj->setProperty("type", "event");
    
    DynamicObject::Ptr c_obj = new DynamicObject();
    c_obj->setProperty("type", type);
    c_obj->setProperty("sampleNum", sampleNum);
    c_obj->setProperty("eventId", eventId);
    c_obj->setProperty("eventChannel", eventChannel);
    obj->setProperty("content", var(c_obj));
    obj->setProperty("dataSize", numBytes);
    
    var json (obj);
    String s = JSON::toString(json);
    void *headerData = (void *)s.toRawUTF8();
    size_t headerSize = s.length();
    zmq_msg_t messageHeader;
    zmq_msg_init_size(&messageHeader, headerSize);
    memcpy(zmq_msg_data(&messageHeader), headerData, headerSize);
    if(numBytes == 0)
    {
        size = zmq_msg_send(&messageHeader, socket, 0);
        jassert(size != -1);
        zmq_msg_close(&messageHeader);
    }
    else
    {
        size = zmq_msg_send(&messageHeader, socket, ZMQ_SNDMORE);
        jassert(size != -1);
        zmq_msg_close(&messageHeader);
        zmq_msg_t message;
        zmq_msg_init_size(&message, numBytes);
        memcpy(zmq_msg_data(&message), eventData, numBytes);
        int size_m = zmq_msg_send(&message, socket, 0);
        jassert(size_m);
        size += size_m;
        zmq_msg_close(&message);
    }
    return size;
}

template<typename T> int ZmqInterface::sendParam(String name, T value)
{
    int size;
    
    messageNumber++;
    
//    MemoryOutputStream jsonHeader;
//    jsonHeader << "{ \"messageNo\": " << messageNumber << "," << newLine;
//    jsonHeader << "  \"type\": \"param\"," << newLine;
//    jsonHeader << " \"content\": " << newLine;
//    jsonHeader << " { \"" << name << "\": " << value  << newLine;
//    jsonHeader << "}," << newLine;
//    jsonHeader << " \"dataSize\": " << 0 << newLine;
//    jsonHeader << "}";
    
    
    DynamicObject::Ptr obj = new DynamicObject();
    
    obj->setProperty("messageNo", messageNumber);
    obj->setProperty("type", "event");
    DynamicObject::Ptr c_obj = new DynamicObject();
    c_obj->setProperty(name, value);
    
    obj->setProperty("content", var(c_obj));
    obj->setProperty("dataSize", 0);
    
    var json (obj);
    String s = JSON::toString(json);
    void *headerData = (void *)s.toRawUTF8();
    size_t headerSize = s.length();
    
    zmq_msg_t messageHeader;
    zmq_msg_init_size(&messageHeader, headerSize);
    memcpy(zmq_msg_data(&messageHeader), headerData, headerSize);
    size = zmq_msg_send(&messageHeader, socket, 0);
    jassert(size != -1);
    zmq_msg_close(&messageHeader);
    
    return size;
}

AudioProcessorEditor* ZmqInterface::createEditor()
{
    
    //        std::cout << "in PythonEditor::createEditor()" << std::endl;
    editor = new GenericEditor(this, true); //TODO change it into something specific
    return editor;
}

bool ZmqInterface::isReady()
{
    return true;
}

void ZmqInterface::setParameter(int parameterIndex, float newValue)
{
    editor->updateParameterButtons(parameterIndex);
    
    //Parameter& p =  parameters.getReference(parameterIndex);
    //p.setValue(newValue, 0);
    
    //threshold = newValue;
    
    //std::cout << float(p[0]) << std::endl;
    editor->updateParameterButtons(parameterIndex);
}

void ZmqInterface::resetConnections()
{
    nextAvailableChannel = 0;
    
    wasConnected = false;

    return;
}

void ZmqInterface::process(AudioSampleBuffer& buffer,
                           MidiBuffer& events)
{
    if(!socket)
        createDataSocket();
    
    sendData(*(buffer.getArrayOfWritePointers()), buffer.getNumChannels(), buffer.getNumSamples(), getNumSamples(0));
    
}

void ZmqInterface::updateSettings()
{
    
}


