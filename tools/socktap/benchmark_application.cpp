#include "benchmark_application.hpp"
#include <chrono>
#include <iostream>
#include <vanetza/asn1/cam.hpp>
#include <vanetza/asn1/denm.hpp>
#include <vanetza/asn1/packet_visitor.hpp>
#include <vanetza/facilities/cam_functions.hpp>
#include <vanetza/facilities/denm_functions.hpp>
#include <boost/units/cmath.hpp>
#include <exception>
#include <functional>
#include <mosquitto.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Benchmark application counts all incoming messages and calculates the message rate.

/*
 *  Indirizzo host del Broker
 */ 
#define mqtt_host "broker.hivemq.com"

/*
 * Porta di connesssione del Broker standard 1883
 */ 
#define mqtt_port 1883

// This is a very simple CA application sending CAMs at a fixed rate.


// Benchmark application counts all incoming messages and calculates the message rate.
struct mosquitto *mosq;
static int run = 1;

using namespace std::chrono;
using namespace vanetza;
using namespace vanetza::facilities;

BenchmarkApplication::BenchmarkApplication(boost::asio::io_service& io) :
    m_timer(io), m_interval(std::chrono::seconds(1))
{
    schedule_timer();
}

BenchmarkApplication::PortType BenchmarkApplication::port()
{
    return host_cast<uint16_t>(0);
}

Application::PromiscuousHook* BenchmarkApplication::promiscuous_hook()
{
    return this;
}

void BenchmarkApplication::tap_packet(const DataIndication& indication, const UpPacket& packet)
{
	asn1::PacketVisitor<asn1::Cam> visitorc;
    std::shared_ptr<const asn1::Cam> cam = boost::apply_visitor(visitorc, packet);
 
   

    asn1::PacketVisitor<asn1::Denm> visitor;
    std::shared_ptr<const asn1::Denm> denm = boost::apply_visitor(visitor, packet);
     if(! cam && ! denm)
     {
     std::cout << "Application received a packet with " << "broken" << " content" << std::endl;
     }
    else if(cam)
    {
         std::cout << "Application received a CAM packet with " << "decodable" << " content" << std::endl;
    }
    else 
        std::cout << "Application received a DENM packet with " << "decodable" << " content" << std::endl;

    if (denm && print_rx_msg_) {
        std::cout << "Received DENM contains\n";
        print_indented(std::cout, *denm, "  ", 1);
    }


    else if (cam && print_rx_msg_) {
		int n= 0;
        std::cout << "Received CAM contains\n";
       // print_indented(std::cout, *cam, "  ", 1);
        std::stringstream ss;
        print_indented(ss, *cam, "  ", 1);
        char test[850];
        ss.read(test,850);
        printf("%s\n",test);
        
        connetti(test);

    }
	
    ++m_received_messages;
}

void BenchmarkApplication::indicate(const DataIndication& indication, UpPacketPtr packet)
{
    // do nothing here
}

void BenchmarkApplication::schedule_timer()
{
    m_timer.expires_from_now(m_interval);
    m_timer.async_wait(std::bind(&BenchmarkApplication::on_timer, this, std::placeholders::_1));
}

void BenchmarkApplication::on_timer(const boost::system::error_code& ec)
{
    if (ec == boost::asio::error::operation_aborted) {
        return;
    }

    std::cout << "Received " << m_received_messages << " messages/second" << std::endl;
    m_received_messages = 0;

    schedule_timer();
}

void  connect_callback(struct mosquitto *mosq, void *obj, int result) 
{
    printf("callback, connected with return code  rc=%d\n", result);
}

void publish_callback(struct mosquitto *mosq, void *userdata, int mid) 
{
    
    printf("message published\n");
    
    /* 
     * Dopo l'invio del messaggio ci disconnettiamo dal server
     */
    mosquitto_disconnect(mosq); 
}

void handle_signal(int s) 
{
    run = 0;
    printf("Signal Handler\n");
}

int BenchmarkApplication::connetti(char*payload){
	
	char id[24];
    
    char topic[] = "test/topic";
 
    struct mosquitto *mosq;
    int rc = 0;
  

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    mosquitto_lib_init(); 

    memset(id, 0, 24);
    snprintf(id, 23, "%d", getpid());

    mosq = mosquitto_new(id, true, 0); 
    
		if(mosq){
			

        mosquitto_connect_callback_set(mosq, connect_callback); 
       
        mosquitto_publish_callback_set(mosq, publish_callback);

        rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 15);
	
        mosquitto_publish(mosq,NULL,topic,strlen(payload),(void*)payload,1,false);  
        
        mosquitto_disconnect(mosq); 
        
       
        
       //sleep(60);
        
        
        
           // printf("LOOP, rc=%d\n", rc);
          
            if(run && rc){
                printf("connection error!\n");
                sleep(5);
                
            }
    }
     mosquitto_publish_callback_set(mosq, publish_callback);
    mosquitto_disconnect(mosq); 
  
    return rc;
}

void BenchmarkApplication::print_received_message(bool flag)
{
   
    print_rx_msg_ = flag;
    
}
