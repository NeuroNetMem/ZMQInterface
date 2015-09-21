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
#include <time.h>

#include "ZmqInterface.h"

#define DEBUG_ZMQ
const int MAX_MESSAGE_LENGTH = 64000;

struct EventData {
    uint8 type;
    uint8 eventId;
    uint8 eventChannel;
    uint8 numBytes;
    int sampleNum;
    time_t eventTime;
    char application[256];
    char uuid[256];
    bool isEvent;
};

ZmqInterface::ZmqInterface(const String &processorName)
    : GenericProcessor(processorName), Thread("Zmq thread")
{
    createContext();
    threadRunning = false;
    openListenSocket();
    openKillSocket();
    openPipeOutSocket();
    
    // TODO initialize structures to keep track of apps
    
    
}

ZmqInterface::~ZmqInterface()
{
    threadRunning = false;
    // send the kill signal to the thread
    
    zmq_msg_t messageEnvelope;
    zmq_msg_init_size(&messageEnvelope, strlen("STOP")+1);
    memcpy(zmq_msg_data(&messageEnvelope), "STOP", strlen("STOP")+1);
    zmq_msg_send(&messageEnvelope, killSocket, 0);
    zmq_msg_close(&messageEnvelope);
    
    stopThread(200); // this is probably overkill but won't hurt
    closeDataSocket();
    zmq_close(killSocket);
    zmq_close(pipeOutSocket);
    
    sleep(500);
    
    if(context)
    {
        zmq_ctx_destroy(context);
        context = 0;
    }
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
        String urlstring;
        urlstring = String("tcp://*:") + String(dataPort);
        std::cout << urlstring << std::endl;
        int rc = zmq_bind(socket, urlstring.toRawUTF8());
        if(rc)
        {
            std::cout << "couldn't open data socket" << std::endl;
            std::cout << zmq_strerror(zmq_errno()) << std::endl;
            assert(false);
        }
        
    }
    return 0;
}

int ZmqInterface::closeDataSocket()
{
    if(socket)
    {
        std::cout << "close data socket" << std::endl;
        int rc = zmq_close(socket);
        assert(rc==0);
        socket = 0;
    }
    return 0;
}

void ZmqInterface::openListenSocket()
{
    startThread();
}

int ZmqInterface::closeListenSocket()
{
    int rc = 0;
         if(listenSocket)
        {
            std::cout << "close listen socket" << std::endl;
            rc = zmq_close(listenSocket);
            listenSocket = 0;
        }
    
    return rc;
}

void ZmqInterface::openKillSocket()
{
    killSocket = zmq_socket(context, ZMQ_PAIR);
    zmq_connect(killSocket, "inproc://zmqthreadcontrol");
}

void ZmqInterface::openPipeOutSocket()
{
    pipeOutSocket = zmq_socket(context, ZMQ_PAIR);
    zmq_bind(pipeOutSocket, "inproc://zmqthreadpipe");
}

void ZmqInterface::run()
{
    listenSocket = zmq_socket(context, ZMQ_REP);
    String urlstring;
    urlstring = String("tcp://*:") + String(listenPort);
    int rc = zmq_bind(listenSocket, urlstring.toRawUTF8()); // give the chance to change the port
    assert(rc == 0);
    threadRunning = true;
    char* buffer = new char[MAX_MESSAGE_LENGTH];

    int size;

    controlSocket = zmq_socket(context, ZMQ_PAIR);
    zmq_bind(controlSocket, "inproc://zmqthreadcontrol");
    
    pipeInSocket = zmq_socket(context, ZMQ_PAIR);
    zmq_connect(pipeInSocket, "inproc://zmqthreadpipe");
    
    zmq_pollitem_t items [] = {
        { listenSocket, 0, ZMQ_POLLIN, 0 },
        { controlSocket, 0, ZMQ_POLLIN, 0 }
    };
    

    while(threadRunning && (!threadShouldExit()))
    {
        zmq_poll (items, 2, -1);
        if(items[0].revents & ZMQ_POLLIN)
        {
            size = zmq_recv(listenSocket, buffer, MAX_MESSAGE_LENGTH-1, 0);
            buffer[size] = 0;
            std::cout << "received something" << std::endl;
            std::cout << buffer << std::endl;
            

            
            if(size < 0)
            {
                std::cout << "failed in receiving listen socket" << std::endl;
                std::cout << zmq_strerror(zmq_errno()) << std::endl;
                assert(false);
            }
            var v;
#ifdef ZMQ_DEBUG
            std::cout << "in listening thread: " << String(buffer) << std::endl;
#endif
            Result rs = JSON::parse(String(buffer), v);
            bool ok = rs.wasOk();


            EventData ed;
            String app = v["application"];
            String appUuid = v["uuid"];
            std::cout << "event from app " << app << " with UUID " << appUuid << std::endl;
            strncpy(ed.application, app.toRawUTF8(),  255);
            strncpy(ed.uuid, appUuid.toRawUTF8(), 255);
            ed.eventTime = time(NULL);
            
            String evT = v["type"];
            
            if(evT == "event")
            {
                ed.isEvent = true;
                ed.eventChannel = (int)v["event"]["event_channel"];
                ed.eventId = (int)v["event"]["event_id"];
                ed.numBytes = 0; // TODO  allow for event data
                ed.sampleNum = (int)v["event"]["sample_num"];
                ed.type = (int)v["event"]["type"];
                std::cout << "chan " << (int)ed.eventChannel << " id " << (int)ed.eventId << " sample n "
                    << (int)ed.sampleNum << " type " << (int)ed.type <<
                    std::endl;
            }
            else
            {
                ed.isEvent = false;
            }
            
            // TODO allow for event data
            
            
            
            // TODO send it to process thread
            
            zmq_msg_t message;
            zmq_msg_init_size(&message, sizeof(EventData));
            memcpy(zmq_msg_data(&message), &ed, sizeof(EventData));
            int size_m = zmq_msg_send(&message, socket, 0);
            jassert(size_m);
            size += size_m;
            zmq_msg_close(&message);
            
            // send response
            String response;
            if(ok)
            {
                response = String("message correctly parsed");
            }
            else
            {
                response = String("JSON message could not be read");
            }
            zmq_send(listenSocket, response.getCharPointer(), response.length(), 0);
            if((!threadRunning) || threadShouldExit())
                break; // we're exiting

        }
        else
            break;
        
    }
    closeListenSocket();
    
    // TODO close the pipe out socket
    zmq_close(controlSocket);
    delete buffer;
    threadRunning = false;
    return;
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
    

    DynamicObject::Ptr obj = new DynamicObject();
    
    int mn = messageNumber;
    obj->setProperty("message_no", mn);
    obj->setProperty("type", "data");
    
    DynamicObject::Ptr c_obj = new DynamicObject();
    
    c_obj->setProperty("n_channels", nChannels);
    c_obj->setProperty("n_samples", nSamples);
    c_obj->setProperty("n_real_samples", nRealSamples);
    
    obj->setProperty("content", var(c_obj));
    obj->setProperty("dataSize", (int)(nChannels * nSamples * sizeof(float)));
    
    var json(obj);
    
    String s = JSON::toString(json);
    void *headerData = (void *)s.toRawUTF8();
    

    size_t headerSize = s.length();
    
    zmq_msg_t messageEnvelope;
    zmq_msg_init_size(&messageEnvelope, strlen("DATA")+1);
    memcpy(zmq_msg_data(&messageEnvelope), "DATA", strlen("DATA")+1);
    int size = zmq_msg_send(&messageEnvelope, socket, ZMQ_SNDMORE);
    jassert(size != -1);
    zmq_msg_close(&messageEnvelope);
    
    zmq_msg_t messageHeader;
    zmq_msg_init_size(&messageHeader, headerSize);
    memcpy(zmq_msg_data(&messageHeader), headerData, headerSize);
    size = zmq_msg_send(&messageHeader, socket, ZMQ_SNDMORE);
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


int ZmqInterface::sendEvent( uint8 type,
                             int sampleNum,
                             uint8 eventId,
                             uint8 eventChannel,
                             uint8 numBytes,
                             const uint8* eventData)
{
    int size;
    
    messageNumber++;
    
    DynamicObject::Ptr obj = new DynamicObject();
    
    obj->setProperty("message_no", messageNumber);
    obj->setProperty("type", "event");
    
    DynamicObject::Ptr c_obj = new DynamicObject();
    c_obj->setProperty("type", type);
    c_obj->setProperty("sample_num", sampleNum);
    c_obj->setProperty("event_id", eventId);
    c_obj->setProperty("event_channel", eventChannel);
    obj->setProperty("content", var(c_obj));
    obj->setProperty("data_size", numBytes);
    
    var json (obj);
    String s = JSON::toString(json);
    void *headerData = (void *)s.toRawUTF8();
    size_t headerSize = s.length();
    
    
    zmq_msg_t messageEnvelope;
    zmq_msg_init_size(&messageEnvelope, strlen("EVENT")+1);
    memcpy(zmq_msg_data(&messageEnvelope), "EVENT", strlen("EVENT")+1);
    size = zmq_msg_send(&messageEnvelope, socket, ZMQ_SNDMORE);
    jassert(size != -1);
    zmq_msg_close(&messageEnvelope);
    
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
    
    obj->setProperty("message_no", messageNumber);
    obj->setProperty("type", "event");
    DynamicObject::Ptr c_obj = new DynamicObject();
    c_obj->setProperty(name, value);
    
    obj->setProperty("content", var(c_obj));
    obj->setProperty("data_size", 0);
    
    var json (obj);
    String s = JSON::toString(json);
    void *headerData = (void *)s.toRawUTF8();
    size_t headerSize = s.length();
    
    zmq_msg_t messageEnvelope;
    zmq_msg_init_size(&messageEnvelope, strlen("PARAM")+1);
    memcpy(zmq_msg_data(&messageEnvelope), "PARAM", strlen("PARAM")+1);
    size = zmq_msg_send(&messageEnvelope, socket, ZMQ_SNDMORE);
    jassert(size != -1);
    zmq_msg_close(&messageEnvelope);
    
    
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

void ZmqInterface::handleEvent(int eventType, MidiMessage& event, int sampleNum)
{
    const uint8* dataptr = event.getRawData();
    int size = event.getRawDataSize();
    uint8 numBytes;
    if(size > 6)
        numBytes = size - 6;
    else
        numBytes = 0;
    int eventId = *(dataptr+2);
    int eventChannel = *(dataptr+3);
    
    sendEvent(eventType,
              sampleNum,
              eventId,
              eventChannel,
              numBytes,
              dataptr+6);
}

int ZmqInterface::receiveEvents(MidiBuffer &events)
{
    // TODO
    return 0;
}

void ZmqInterface::process(AudioSampleBuffer& buffer,
                           MidiBuffer& events)
{
    if(!socket)
        createDataSocket();

    checkForEvents(events); // see if we got any TTL events

    sendData(*(buffer.getArrayOfWritePointers()), buffer.getNumChannels(), buffer.getNumSamples(), getNumSamples(0));
    
    receiveEvents(events);
    
}

void ZmqInterface::updateSettings()
{
    
}


