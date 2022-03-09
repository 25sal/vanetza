#include <iostream>

#include "denm_application.hpp"
#include <vanetza/btp/ports.hpp>
#include <vanetza/asn1/denm.hpp>
#include <vanetza/asn1/packet_visitor.hpp>
#include <vanetza/facilities/denm_functions.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <chrono>
#include <exception>
#include <functional>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <vanetza/asn1/security/CountryAndRegions.h>

// This is a very simple CA application sending CAMs at a fixed rate.

using namespace vanetza;
using namespace vanetza::facilities;
using namespace std::chrono;
using namespace vanetza::security;


DenmApplication::DenmApplication(PositionProvider& positioning, Runtime& rt) :
    positioning_(positioning), runtime_(rt), denm_interval_(seconds(1))
{
        
    myfile.open("a.txt");
    schedule_timer();
}




void DenmApplication::set_interval(Clock::duration interval)
{
    denm_interval_ = interval;
    runtime_.cancel(this);
    schedule_timer();
}

void DenmApplication::print_generated_message(bool flag)
{
    print_tx_msg_ = flag;
}

void DenmApplication::print_received_message(bool flag)
{
    print_rx_msg_ = flag;
}

DenmApplication::PortType DenmApplication::port()
{
    return btp::ports::DENM;
}

void DenmApplication::indicate(const DataIndication& indication, UpPacketPtr packet)
{
    asn1::PacketVisitor<asn1::Denm> visitor;
    std::shared_ptr<const asn1::Denm> denm = boost::apply_visitor(visitor, *packet);

    std::cout << "DENM application received a packet with " << (denm ? "decodable" : "broken") << " content" << std::endl;
    if (denm && print_rx_msg_) {
        std::cout << "Received DENM contains\n";
        print_indented(std::cout, *denm, "  ", 1);
    }
}

void DenmApplication::schedule_timer()
{
    runtime_.schedule(denm_interval_, std::bind(&DenmApplication::on_timer, this, std::placeholders::_1), this);
}

void DenmApplication::on_timer(Clock::time_point)
{

 if(myfile.is_open() && myfile.peek()!=EOF)
  {

    long int linen, lat, lon, altVal, altConf, sType, infQuality, causecode, sub, evePos, distance, segID, timest, country; 
    int valDur;

    myfile >> linen;
    if(linen<0) 
    {
        myfile.close();
        return;
    }
    myfile >> lat;
    myfile >> lon;
    myfile >> altVal;
    myfile >> altConf;
    myfile >> valDur;
    myfile >> sType;
    myfile >> infQuality;
    myfile >> causecode;
    myfile >> sub;
    myfile >> evePos;
    myfile >> distance;
    myfile >> segID;
    myfile >> timest;
    myfile >> country;


    cout << lat << lon << altVal << altConf << valDur << sType << infQuality <<" causecode:"<< causecode<<" sub: " << sub << evePos << distance << segID << timest << "country: "<<country << endl;
    
  
    schedule_timer();

    vanetza::asn1::Denm message;
    
    ItsPduHeader_t& header = message->header;
    header.protocolVersion = 2;
    header.messageID = ItsPduHeader__messageID_denm;
    header.stationID = 1; // some dummy value

    DecentralizedEnvironmentalNotificationMessage_t& denm = message->denm;
    
    auto position = positioning_.position_fix();

    if (!position.confidence) {
        std::cerr << "Skipping DENM, because no good position is available, yet." << std::endl;
        return;
    }
   
    //const auto time_now = duration_cast<milliseconds>(runtime_.now().time_since_epoch());
    const auto time_now  = duration_cast<seconds>(system_clock::now().time_since_epoch());
    uint64_t gen_delta_time = time_now.count();
    ManagementContainer_t& management = denm.management;
    management.actionID.originatingStationID = 1;
    management.actionID.sequenceNumber = 2220;
    //management.detectionTime = 4; //TimestampIts_t ?
    //management.referenceTime = 3; //TimestampIts_t ?
    
    
    
    /*
	// Seems like ASN1 supports 32 bit int (strange) and timestamp needs 42 bits.
	TimestampIts_t* timestamp = vanetza::asn1::allocate<TimestampIts_t>();

	//memset(timestamp,0,sizeof(TimestampIts_t));
	memcpy(timestamp, &time_now, sizeof(time_now)); //6bytes?
	
	//occorre inserire il tempo in timestampt
    message->denm.management.detectionTime = *timestamp;
    message->denm.management.referenceTime = *timestamp;
    */
    //memcpy(&management.referenceTime, &gen_delta_time, sizeof(time_now));
   

    
    
    int ret1 =  asn_uint642INTEGER((INTEGER_t*)&management.referenceTime, gen_delta_time);
    int ret2 =  asn_uint642INTEGER((INTEGER_t*)&management.detectionTime, gen_delta_time);
    assert(ret1+ret2==0);
    /*SALVATORE**/
    
    PositionFix fix;
    fix.timestamp = runtime_.now();
    fix.latitude = (double) lat * units::degree;
    fix.longitude = (double) lon * units::degree;
    fix.confidence.semi_major =  5.0 * units::si::meter;
    fix.confidence.semi_minor = fix.confidence.semi_major;
    
    

    //management.eventPosition.latitude = lat ;
    //management.eventPosition.longitude = lon;
    //management.eventPosition.altitude.altitudeValue = altVal;
    //management.eventPosition.altitude.altitudeConfidence = altConf;
    //management.eventPosition.roadSegmentReferenceID.id = segID;
    message->denm.management.relevanceDistance = vanetza::asn1::allocate<RelevanceDistance_t>();
    /*
    if(distance > 10000) 
    {}
    elseif(distance < 5000)
    4,	 lessThan1000m(4) 
	1,	 lessThan100m(1) 
	6,	 lessThan10km(6) 
	2,	 lessThan200m(2) 
	3,   lessThan500m(3) 
	0,	 lessThan50m(0) 
	5,	 lessThan5km(5) 
	7	 over10km(7) 
    */

    *(message->denm.management.relevanceDistance) = RelevanceDistance_lessThan1000m;
    message->denm.management.validityDuration = vanetza::asn1::allocate<ValidityDuration_t>();
    *(message->denm.management.validityDuration) = valDur;
    management.stationType = sType;
    (fix, management.eventPosition);
    
    
    
    //from artery TrafficJamUseCase
    //message->denm.management.relevanceDistance = vanetza::asn1::allocate<RelevanceDistance_t>();
    //*(message->denm.management.relevanceDistance) = RelevanceDistance_lessThan1000m;
    message->denm.management.relevanceTrafficDirection = vanetza::asn1::allocate<RelevanceTrafficDirection_t>();
    *(message->denm.management.relevanceTrafficDirection) = RelevanceTrafficDirection_upstreamTraffic;
    
    message->denm.management.stationType = StationType_unknown; // TODO retrieve type from SUMO
    //from artery
    message->denm.situation = vanetza::asn1::allocate<SituationContainer_t>();
    message->denm.situation = vanetza::asn1::allocate<SituationContainer_t>();
    /*
    switch(infQuality){0,1,7: InformationQuality_highest} ...
    */
    message->denm.situation->informationQuality = InformationQuality_highest;
    message->denm.situation->eventType.causeCode = 1;
    
    /*
    Gli interi 
    */
    message->denm.situation->eventType.causeCode = causecode;//CauseCodeType_accident;
    message->denm.situation->eventType.subCauseCode = sub;
    //*(message->denm.situation->eventHistory.eventPosition.deltaReferencePosition) = evePos;
    //situation.eventHistory.eventPosition =  DeltaReferencePosition; ? 
    //situation.eventHistory.eventDeltaTime = PathDeltaTime OPTIONAL; ?
    //situation.eventHistory.informationQuality = InformationQuality; ?
    

    std::string error;
    
    if (!message.validate(error)) {
        throw std::runtime_error("Invalid high frequency DENM: %s" + error);
    }

    if (print_tx_msg_) {
  /*Json::Reader reader;
    Json::Value node;
    
    std::ifstream node_file("node.json", std::ifstream::binary);
node_file >> node;

cout<<node; */
    
    	

        print_indented(std::cout, message, "  ", 1);
	
    }

    DownPacketPtr packet { new DownPacket() };
    packet->layer(OsiLayer::Application) = std::move(message);

    DataRequest request;
    request.its_aid = aid::CA;
    
    request.transport_type = geonet::TransportType::SHB;
    request.communication_profile = geonet::CommunicationProfile::ITS_G5;
    auto confirm = Application::request(request, std::move(packet));

    if (!confirm.accepted()) {
        throw std::runtime_error("DENM application data request failed");
    }
   
  }
  else{
  cout<<"No DENM messages"<<endl;
  return;
  }
}
