#include "beacon.h"

namespace Cosmos {
    namespace Support {
        int32_t Beacon::Init(uint16_t short_beacon, uint16_t long_beacon) {
            // Beacon and SMS message byte size limits
            short_beacon_size = short_beacon;
            long_beacon_size = long_beacon;
            interval = 1.;
            pattern_idx = 0;
//            agent = calling_agent;
            return 0;
        }

        int32_t Beacon::add_beacon(const string& name, uint8_t type, size_t size) {
            beacon_size[name] = size;
            beacon_typeID[name] = type;
            // TODO: create invalid beacon size error code
            return 0;
        }

        int32_t Beacon::EncodeBinary(TypeId type, cosmosstruc* cinfo, vector<uint8_t> &data)
        {
            if (cinfo == nullptr)
            {
                return GENERAL_ERROR_NULLPOINTER;
            }

            cinfo->node.met = (currentmjd() - cinfo->node.utcstart);
            data.clear();
            this->type = type;
            switch (type)
            {
            case TypeId::CPU1BeaconS:
                if (cinfo->devspec.cpu.size())
                {
                    cpu1_beacons beacon;
                    beacon.met = cinfo->node.met;
                    beacon.load = cinfo->devspec.cpu[0].load;
                    beacon.memory = cinfo->devspec.cpu[0].gib;
                    beacon.disk = cinfo->devspec.cpu[0].storage * 100. + .5;
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+sizeof(beacon));
                }
                break;
            case TypeId::CPU2BeaconS:
                if (cinfo->devspec.cpu.size())
                {
                    cpu2_beacons beacon;
                    beacon.met = cinfo->node.met;
                    beacon.uptime = cinfo->devspec.cpu[0].uptime;
                    beacon.bootcount = cinfo->devspec.cpu[0].boot_count;
                    beacon.initialdate = utc2unixseconds(cinfo->node.utcstart) + .5;
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+sizeof(beacon));
                }
                break;
            case TypeId::TsenBeaconS:
                if (cinfo->devspec.tsen.size() >= 3)
                {
                    tsen_beacons beacon;
                    beacon.met = cinfo->node.met;
                    for (uint16_t i=0; i<3; ++i)
                    {
                        beacon.temp[i] = cinfo->devspec.tsen[i].temp;
                    }
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+sizeof(beacon));
                }
                break;
            case TypeId::EPSCPUBeaconS:
                for (uint16_t i=0; i<cinfo->devspec.cpu.size(); ++i)
                {
                    if (cinfo->devspec.cpu[i].name.find("eps") != string::npos)
                    {
                        epscpu_beacons beacon;
                        beacon.met = cinfo->node.met;
                        beacon.volt = cinfo->devspec.cpu[i].volt;
                        beacon.amp = cinfo->devspec.cpu[i].amp;
                        beacon.temp = cinfo->devspec.cpu[i].temp;
                        data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+sizeof(beacon));
                        break;
                    }
                }
                break;
            case TypeId::EPSPVBeaconS:
                {
                    epsbcreg_beacons beacon;
                    beacon.met = cinfo->node.met;
                    for (uint16_t i=0; i<cinfo->devspec.pvstrg.size(); ++i)
                    {
                        beacon.volt += cinfo->devspec.pvstrg[i].volt;
                        beacon.amp += cinfo->devspec.pvstrg[i].amp;
                        beacon.temp += cinfo->devspec.pvstrg[i].temp;
                    }
                    if (cinfo->devspec.pvstrg.size())
                    {
                        beacon.volt /= cinfo->devspec.pvstrg.size();
                        beacon.temp /= cinfo->devspec.pvstrg.size();
                        data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+sizeof(beacon));
                    }
                }
                break;
            case TypeId::EPSSWCHBeaconS:
                {
                    epsswch_beacons beacon;
                    beacon.met = cinfo->node.met;
                    for (uint16_t i=0; i<cinfo->devspec.swch.size(); ++i)
                    {
                        beacon.volt += cinfo->devspec.swch[i].volt;
                        beacon.amp += cinfo->devspec.swch[i].amp;
                        beacon.temp += cinfo->devspec.swch[i].temp;
                    }
                    if (cinfo->devspec.swch.size())
                    {
                        beacon.volt /= cinfo->devspec.swch.size();
                        beacon.temp /= cinfo->devspec.swch.size();
                        data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+sizeof(beacon));
                    }
                }
                break;
            case TypeId::EPSBATTBeaconS:
                {
                    epsbatt_beacons beacon;
                    beacon.met = cinfo->node.met;
                    for (uint16_t i=0; i<cinfo->devspec.batt.size(); ++i)
                    {
                        beacon.volt += cinfo->devspec.batt[i].volt;
                        beacon.amp += cinfo->devspec.batt[i].amp;
                        beacon.temp += cinfo->devspec.batt[i].temp;
                    }
                    if (cinfo->devspec.batt.size())
                    {
                        beacon.volt /= cinfo->devspec.batt.size();
                        beacon.temp /= cinfo->devspec.batt.size();
                        data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+sizeof(beacon));
                    }
                }
                break;
            case TypeId::CPUBeaconL:
                if (cinfo->devspec.cpu.size())
                {
                    cpus_beacon beacon;
                    beacon.met = cinfo->node.met;
                    beacon.initialdate = utc2unixseconds(cinfo->node.utcstart) + .5;
                    uint16_t cpucount = cinfo->devspec.cpu.size() < cpu_count?cinfo->devspec.cpu.size():cpu_count;
                    for (uint16_t i=0; i<cpucount; ++i)
                    {
                        beacon.cpu[i].uptime = cinfo->devspec.cpu[i].uptime;
                        beacon.cpu[i].bootcount = cinfo->devspec.cpu[i].boot_count;
                        if (cinfo->devspec.cpu[i].load < 65.536)
                        {
                            beacon.cpu[i].mload = cinfo->devspec.cpu[i].load * 1000. + .5;
                        }
                        else
                        {
                            beacon.cpu[i].mload = 65535;
                        }
                        beacon.cpu[i].mmemory = 1000. * (cinfo->devspec.cpu[i].gib / cinfo->devspec.cpu[i].maxgib) + .5;
                        beacon.cpu[i].mdisk = cinfo->devspec.cpu[i].storage * 1000. + .5;
                        beacon.cpu[i].ctemp = cinfo->devspec.cpu[i].temp * 100. + .5;
//                        printf("EB CPU %u %s: pidx=%u uptime=%u name=%s\n", i, cinfo->devspec.cpu[i].name.c_str(), cinfo->devspec.cpu[i].pidx, cinfo->devspec.cpu[i].uptime, cinfo->pieces[cinfo->devspec.cpu[i].pidx].name.c_str());
                        fflush(stdout);
                    }
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+9+cpucount*sizeof(cpu_beacon));
                }
                break;
            case TypeId::TsenBeaconL:
                if (cinfo->devspec.tsen.size())
                {
                    tsen_beacon beacon;
                    beacon.met = cinfo->node.met;
                    uint16_t tsencount = cinfo->devspec.tsen.size() < tsen_count?cinfo->devspec.tsen.size():tsen_count;
                    for (uint16_t i=0; i<tsencount; ++i)
                    {
                        beacon.ctemp[i] = cinfo->devspec.tsen[i].temp * 100. + .5;
//                        printf("EB TSEN %u %s: pidx=%u temp=%f\n", i, cinfo->devspec.tsen[i].name.c_str(), cinfo->devspec.tsen[i].pidx, cinfo->devspec.tsen[i].temp);
                    }
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+5+tsencount*2);
                }
                break;
            case TypeId::EPSSWCHBeaconL:
                if (cinfo->devspec.swch.size())
                {
                    epsswchs_beacon beacon;
                    beacon.met = cinfo->node.met;
                    uint16_t swchcount = cinfo->devspec.swch.size() < epsswch_count?cinfo->devspec.swch.size():epsswch_count;
                    for (uint16_t i=0; i<swchcount; ++i)
                    {
                        beacon.swch[i].mamp = cinfo->devspec.swch[i].amp * 1000. + .5;
                        beacon.swch[i].mvolt = cinfo->devspec.swch[i].volt * 1000. + .5;
//                        printf("EB SWCH %u %s: pidx=%u volt=%f amp=%f\n", i, cinfo->devspec.swch[i].name.c_str(), cinfo->devspec.swch[i].pidx, cinfo->devspec.swch[i].volt, cinfo->devspec.swch[i].amp);
                    }
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+5+swchcount*sizeof(epsswch_beacon));
                }
                break;
            case TypeId::EPSBCREGBeaconL:
                if (cinfo->devspec.swch.size())
                {
                    epsbcregs_beacon beacon;
                    beacon.met = cinfo->node.met;
                    uint16_t pvcount = cinfo->devspec.bcreg.size() < epsbcreg_count?cinfo->devspec.bcreg.size():epsbcreg_count;
                    for (uint16_t i=0; i<pvcount; ++i)
                    {
                        beacon.bcreg[i].mamp = cinfo->devspec.bcreg[i].amp * 1000. + .5;
                        beacon.bcreg[i].mvolt = cinfo->devspec.bcreg[i].volt * 1000. + .5;
                    }
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+5+pvcount*sizeof(epsbcreg_beacon));
                }
                break;
            case TypeId::EPSBATTBeaconL:
                if (cinfo->devspec.swch.size())
                {
                    epsbatts_beacon beacon;
                    beacon.met = cinfo->node.met;
                    uint16_t battcount = cinfo->devspec.batt.size() < epsbatt_count?cinfo->devspec.batt.size():epsbatt_count;
                    for (uint16_t i=0; i<battcount; ++i)
                    {
                        beacon.batt[i].mamp = cinfo->devspec.batt[i].amp * 1000. + .5;
                        beacon.batt[i].mvolt = cinfo->devspec.batt[i].volt * 1000. + .5;
                        beacon.batt[i].cpercent = cinfo->devspec.batt[i].percentage * 100. + .5;
                        beacon.batt[i].ctemp = cinfo->devspec.batt[i].temp * 100. + .5;
                    }
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+5+battcount*sizeof(epsbatt_beacon));
                }
                break;
            case TypeId::ADCSMTRBeaconL:
                if (cinfo->devspec.mtr.size())
                {
                    adcsmtrs_beacon beacon;
                    beacon.met = cinfo->node.met;
                    uint16_t mtrcount = cinfo->devspec.mtr.size() < adcsmtr_count?cinfo->devspec.mtr.size():adcsmtr_count;
                    for (uint16_t i=0; i<mtrcount; ++i)
                    {
                        beacon.mtr[i].mom = cinfo->devspec.mtr[i].mom * 1000. + .5;
                        beacon.mtr[i].align[0] = cinfo->devspec.mtr[i].align.w * 1000. + .5;
                        beacon.mtr[i].align[1] = cinfo->devspec.mtr[i].align.d.x * 100. + .5;
                        beacon.mtr[i].align[2] = cinfo->devspec.mtr[i].align.d.y * 100. + .5;
                        beacon.mtr[i].align[3] = cinfo->devspec.mtr[i].align.d.z * 100. + .5;
                    }
                    data.insert(data.begin(), (uint8_t*)&beacon, (uint8_t*)&beacon+5+mtrcount*sizeof(adcsmtr_beacon));
                }
                break;
            default:
                return GENERAL_ERROR_OUTOFRANGE;
            }
//            printf("Beacon: Type=%u Size=%lu\n", (uint16_t)type, data.size());
            return data.size();
        }

        int32_t Beacon::Decode(vector<uint8_t> &data, cosmosstruc *cinfo)
        {
            type = (TypeId) data[0];
            if (TypeString.find(type) != TypeString.end())
            {
                switch (type)
                {
                case TypeId::CPUBeaconL:
                    {
                        cpus_beacon beacon;
                        if (data.size() <= sizeof(beacon))
                        {
                            memcpy(&beacon, data.data(), data.size());
                        }
                        else
                        {
                            return GENERAL_ERROR_BAD_SIZE;
                        }
                        cinfo->node.utcstart = unix2utc(beacon.initialdate);
                        double mjd = beacon.met + cinfo->node.utcstart;
                        for (uint16_t i=0; i<(data.size()-9)/COSMOS_SIZEOF(cpu_beacon); ++i)
                        {
                            if (beacon.cpu[i].uptime > 0.)
                            {
                                cinfo->devspec.cpu[i].utc = mjd;
                                cinfo->devspec.cpu[i].uptime = beacon.cpu[i].uptime;
                                cinfo->devspec.cpu[i].boot_count = beacon.cpu[i].bootcount;
                                cinfo->devspec.cpu[i].load = beacon.cpu[i].mload / 1000.;
                                cinfo->devspec.cpu[i].gib = cinfo->devspec.cpu[i].maxgib * (beacon.cpu[i].mmemory / 1000.);
                                cinfo->devspec.cpu[i].storage = beacon.cpu[i].mdisk / 1000.;
                                cinfo->devspec.cpu[i].temp = beacon.cpu[i].ctemp / 100.;
                            }
                        }
                    }
                    break;
                case TypeId::TsenBeaconL:
                    {
                        tsen_beacon beacon;
                        if (data.size() <= sizeof(beacon))
                        {
                            memcpy(&beacon, data.data(), data.size());
                        }
                        else
                        {
                            return GENERAL_ERROR_BAD_SIZE;
                        }
                        double mjd = beacon.met + cinfo->node.utcstart;
                        for (uint16_t i=0; i<(data.size()-5)/2; ++i)
                        {
                            if (beacon.ctemp[i])
                            {
                                cinfo->devspec.tsen[i].utc = mjd;
                                cinfo->devspec.tsen[i].temp = beacon.ctemp[i] / 100.;
                            }
                            printf("D TSEN %u %s: pidx=%u temp=%f\n", i, cinfo->devspec.tsen[i].name.c_str(), cinfo->devspec.tsen[i].pidx, cinfo->devspec.tsen[i].temp);
                        }
                    }
                    break;
                case TypeId::EPSSWCHBeaconL:
                    {
                        epsswchs_beacon beacon;
                        if (data.size() <= sizeof(beacon))
                        {
                            memcpy(&beacon, data.data(), data.size());
                        }
                        else
                        {
                            return GENERAL_ERROR_BAD_SIZE;
                        }
                        double mjd = beacon.met + cinfo->node.utcstart;
                        for (uint16_t i=0; i<(data.size()-5)/COSMOS_SIZEOF(epsswch_beacon); ++i)
                        {
                            cinfo->devspec.swch[i].utc = mjd;
                            cinfo->devspec.swch[i].volt = beacon.swch[i].mvolt / 1000.;
                            cinfo->devspec.swch[i].amp = beacon.swch[i].mamp / 1000.;
                            cinfo->devspec.swch[i].power = cinfo->devspec.swch[i].volt * cinfo->devspec.swch[i].amp;
//                            printf("D SWCH %u %s: pidx=%u volt=%f amp=%f\n", i, cinfo->devspec.swch[i].name.c_str(), cinfo->devspec.swch[i].pidx, cinfo->devspec.swch[i].volt, cinfo->devspec.swch[i].amp);
                        }
                    }
                    break;
                case TypeId::EPSBCREGBeaconL:
                    {
                        epsbcregs_beacon beacon;
                        if (data.size() <= sizeof(beacon))
                        {
                            memcpy(&beacon, data.data(), data.size());
                        }
                        else
                        {
                            return GENERAL_ERROR_BAD_SIZE;
                        }
                        double mjd = beacon.met + cinfo->node.utcstart;
                        for (uint16_t i=0; i<(data.size()-5)/COSMOS_SIZEOF(epsbcreg_beacon); ++i)
                        {
                            cinfo->devspec.bcreg[i].utc = mjd;
                            cinfo->devspec.bcreg[i].volt = beacon.bcreg[i].mvolt / 1000.;
                            cinfo->devspec.bcreg[i].amp = beacon.bcreg[i].mamp / 1000.;
                        }
                    }
                    break;
                case TypeId::EPSBATTBeaconL:
                    {
                        epsbatts_beacon beacon;
                        if (data.size() <= sizeof(beacon))
                        {
                            memcpy(&beacon, data.data(), data.size());
                        }
                        else
                        {
                            return GENERAL_ERROR_BAD_SIZE;
                        }
                        double mjd = beacon.met + cinfo->node.utcstart;
                        for (uint16_t i=0; i<(data.size()-5)/COSMOS_SIZEOF(epsbatt_beacon); ++i)
                        {
                            cinfo->devspec.batt[i].utc = mjd;
                            cinfo->devspec.batt[i].volt = beacon.batt[i].mvolt / 1000.;
                            cinfo->devspec.batt[i].amp = beacon.batt[i].mamp / 1000.;
                            cinfo->devspec.batt[i].power = cinfo->devspec.batt[i].volt * cinfo->devspec.batt[i].amp;
                            cinfo->devspec.batt[i].temp = beacon.batt[i].ctemp / 100.;
                            cinfo->devspec.batt[i].percentage = beacon.batt[i].cpercent / 100.;
                        }
                    }
                    break;

					default: break;
                }
            }
            return 0;
        }

        int32_t Beacon::EncodeJson(TypeId type, cosmosstruc *cinfo, vector<uint8_t>& Contents)
        {
            string scontents;
            int32_t iretn = EncodeJson(type, cinfo, scontents);
            Contents.clear();
            Contents.insert(Contents.begin(), scontents.begin(), scontents.end());
            return iretn;
        }

        int32_t Beacon::EncodeJson(TypeId type, cosmosstruc *cinfo, string& Contents)
        {
            Contents.clear();
                switch (type)
                {
                case TypeId::CPU1BeaconS:
                    {
                        cinfo->node.met = cinfo->devspec.cpu[0].utc - cinfo->node.utcstart;
                        json_out(Contents, "node_met", cinfo);
                        json_out_1d(Contents, "device_cpu_load", 0, cinfo);
                        json_out_1d(Contents, "device_cpu_gib", 0, cinfo);
                        json_out_1d(Contents, "device_disk_gib", 0, cinfo);
                    }
                    break;
                case TypeId::CPU2BeaconS:
                    {
                        cinfo->node.met = cinfo->devspec.cpu[0].utc - cinfo->node.utcstart;
                        json_out(Contents, "node_met", cinfo);
                        json_out_1d(Contents, "device_cpu_uptime", 0, cinfo);
                        json_out_1d(Contents, "device_cpu_boot_count", 0, cinfo);
                        json_out(Contents, "node_utcstart", cinfo);
                    }
                    break;
                case TypeId::TsenBeaconS:
                    {
                        cinfo->node.met = cinfo->devspec.tsen[0].utc - cinfo->node.utcstart;
                        json_out(Contents, "node_met", cinfo);
                        for (uint16_t i=0; i<std::min(static_cast<size_t>(3), cinfo->devspec.tsen.size()); ++i)
                        {
                            json_out_1d(Contents, "device_tsen_temp", i, cinfo);
                        }
                    }
                    break;
                case TypeId::EPSCPUBeaconS:
                    {
                        for (uint16_t i=0; i<cinfo->devspec.cpu.size(); ++i)
                        {
                            if (cinfo->devspec.cpu[i].name.find("eps") != string::npos)
                            {
                                json_out_1d(Contents, "device_cpu_volt", i, cinfo);
                                json_out_1d(Contents, "device_cpu_amp", i, cinfo);
                                json_out_1d(Contents, "device_cpu_temp", i, cinfo);
                                break;
                            }
                        }
                    }
                    break;
                case TypeId::EPSPVBeaconS:
                    {
                    }
                    break;
                case TypeId::EPSSWCHBeaconS:
                    {
                    }
                    break;
                case TypeId::EPSBATTBeaconS:
                    {
                    }
                    break;
                case TypeId::CPUBeaconL:
                    {
                        json_out(Contents, "node_utcstart", cinfo);
                        for (uint16_t i=0; i<cinfo->devspec.cpu.size(); ++i)
                        {
                            json_out_1d(Contents, "device_cpu_uptime", i, cinfo);
                            json_out_1d(Contents, "device_cpu_boot_count", i, cinfo);
                            json_out_1d(Contents, "device_cpu_load", i, cinfo);
                            json_out_1d(Contents, "device_cpu_gib", i, cinfo);
                            json_out_1d(Contents, "device_cpu_storage", i, cinfo);
                        }
                    }
                    break;
                case TypeId::TsenBeaconL:
                    {
                        for (uint16_t i=0; i<cinfo->devspec.tsen.size(); ++i)
                        {
                            json_out_1d(Contents, "device_tsen_temp", i, cinfo);
                        }
                    }
                    break;
                case TypeId::EPSSWCHBeaconL:
                    {
                        for (uint16_t i=0; i<cinfo->devspec.swch.size(); ++i)
                        {
                            json_out_1d(Contents, "device_swch_amp", i, cinfo);
                            json_out_1d(Contents, "device_swch_volt", i, cinfo);
                            json_out_1d(Contents, "device_swch_power", i, cinfo);
                        }
                    }
                    break;
                case TypeId::EPSBCREGBeaconL:
                    {
                        for (uint16_t i=0; i<cinfo->devspec.bcreg.size(); ++i)
                        {
                            json_out_1d(Contents, "device_bcreg_amp", i, cinfo);
                            json_out_1d(Contents, "device_bcreg_volt", i, cinfo);
                            json_out_1d(Contents, "device_bcreg_power", i, cinfo);
                        }
                    }
                    break;
                case TypeId::EPSBATTBeaconL:
                    {
                        for (uint16_t i=0; i<cinfo->devspec.batt.size(); ++i)
                        {
                            json_out_1d(Contents, "device_batt_amp", i, cinfo);
                            json_out_1d(Contents, "device_batt_volt", i, cinfo);
                            json_out_1d(Contents, "device_batt_power", i, cinfo);
                            json_out_1d(Contents, "device_batt_temp", i, cinfo);
                            json_out_1d(Contents, "device_batt_percentage", i, cinfo);
                        }
                    }
                    break;
                default:
                    return GENERAL_ERROR_OUTOFRANGE;
                }
            return 0;
        }

        double Beacon::get_interval() {
            return interval;
        }

        int32_t Beacon::set_pattern(const vector<string>& pattern) {
            std::lock_guard<mutex> lock(send_pattern_mtx);
            // TODO: add pattern verifier
            send_pattern = pattern;

            return 0;
        }

    }
}
