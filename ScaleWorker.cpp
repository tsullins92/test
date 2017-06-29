/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "ScaleWorker.h"
#include "FrmMain.h"
#include "BufferedAsyncSerial.h"
#include <sstream>
#include <chrono>

ScaleWorker::ScaleWorker() :
  m_Mutex(),
  m_shall_stop(false),
  m_has_stopped(false), 
  m_scale_reading("0.0")
{
}

// Accesses to these data are synchronized by a mutex.
// Some microseconds can be saved by getting all data at once, instead of having
// separate get_fraction_done() and get_message() methods.
void ScaleWorker::get_data(Glib::ustring* scale_reading) const
{
  std::lock_guard<std::mutex> lock(m_Mutex);

  if (scale_reading)
    *scale_reading = m_scale_reading;
}


void ScaleWorker::read_scale(FrmMain* caller)
{      
    for(;;)
    {  
       
        //serial connection to scale
        BufferedAsyncSerial scaleSerial("/dev/ttyUSB0",9600);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::lock_guard<std::mutex> lock(m_Mutex);
        {
            string scaleReading = scaleSerial.readStringUntil("\r"); 
            if(!scaleReading.empty()){
                m_scale_reading = scaleReading;
            }
            cout<<"scaleReading = "<<scaleReading<<endl;  
            scaleSerial.close();
            control_active_pumps(m_scale_reading,"60"); 
        }
        caller->notify();   
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}


 void ScaleWorker::control_ard()
{
    try
    {
        BufferedAsyncSerial pumpSerial("/dev/ttyACM0",9600,boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),boost::asio::serial_port_base::character_size(8));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        string pumpReading = pumpSerial.readStringUntil("\r");
        std::lock_guard<std::mutex> lock(m_Mutex);
        {
            cout<<"pumpReading = "<<pumpReading<<endl;
        }
        pumpSerial.close();
        
    }

    catch(boost::system::system_error& e)
    {
        cout<<"Error: "<<e.what()<<endl;
        
    }
}


void ScaleWorker::control_active_pumps(string reading, string target)
{
    stringstream ssin {reading};
    stringstream ssout {""};
    double value;
    double goal = stod(target);
    
    for (char ch;ssin.get(ch);){ 
        if((isdigit(ch))||ch=='.')
        {
            ssout<<ch;
        }
        else if(ch=='\r')
        {
            break;
        }
        
    }
    cout << "ssout = " << ssout.str() << "\n";
    value = stod(ssout.str());
    cout <<"value = " << value << "\n";
    cout << "goal = " << goal << "\n";
    return;
}

void ScaleWorker::stop_work()
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_shall_stop = true;
}

bool ScaleWorker::has_stopped() const
{
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_has_stopped;
}
