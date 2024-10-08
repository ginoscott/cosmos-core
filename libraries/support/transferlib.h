#ifndef TRANSFERLIB2_H
#define TRANSFERLIB2_H

// cosmos includes
#include "support/configCosmos.h"
#include "support/datalib.h"
#include "support/packetcomm.h"

#define PROGRESS_QUEUE_SIZE 256

// Radios are only allowing 253 byte packet. IP/UDP header is 28 bytes.
#define TRANSFER_MAX_PROTOCOL_PACKET 225
#define TRANSFER_MAX_FILENAME 128
#ifndef COSMOS_WIN_BUILD_MSVC
#define TRANSFER_QUEUE_LIMIT ((TRANSFER_MAX_PROTOCOL_PACKET-(COSMOS_SIZEOF(PACKET_TYPE)+COSMOS_SIZEOF(PACKET_NODE_ID_TYPE)+COSMOS_SIZEOF(PACKET_TX_ID_TYPE)+COSMOS_MAX_NAME))/(COSMOS_SIZEOF(PACKET_TX_ID_TYPE)))
#else
#define TRANSFER_QUEUE_LIMIT 100
#endif

namespace Cosmos {
    namespace Support {
        typedef uint8_t PACKET_BYTE;
        typedef uint8_t PACKET_TYPE;
        typedef uint16_t PACKET_CRC;
        typedef uint8_t FILE_PROTOCOL_VERSION_TYPE;
        typedef uint8_t PACKET_NODE_ID_TYPE;
        typedef uint8_t PACKET_TX_ID_TYPE;
        typedef uint16_t PACKET_FILE_CRC_TYPE;
        typedef uint16_t PACKET_CHUNK_SIZE_TYPE;
        typedef uint32_t PACKET_FILE_SIZE_TYPE;
        typedef uint32_t PACKET_UNIXTIME_TYPE;
        const PACKET_FILE_CRC_TYPE PACKET_FILE_CRC_FORCE = 65535;

        //! Increment version whenever the protocol has breaking changes to avoid version mismatches.
        //! This is the version number of the protocol packets that are sent between nodes.
        constexpr uint8_t FILE_TRANSFER_PROTOCOL_VERSION = 3;
        //! This is the version of the .meta and .file files that are created and used by write_meta() and read_meta.
        //!To be treated separately from FILE_TRANSFER_PROTOCOL_VERSION.
        constexpr uint8_t FILE_TRANSFER_METAFILE_VERSION = 99;

        /// Chunk start and end.
        struct file_progress
        {
            PACKET_FILE_SIZE_TYPE chunk_start;
            PACKET_FILE_SIZE_TYPE chunk_end;
        };

        //! All communication file packets to have this header
        struct file_packet_header
        {
            //! File transfer protocol version
            uint8_t version = FILE_TRANSFER_PROTOCOL_VERSION;
            //! ID of origin or destination node
            PACKET_NODE_ID_TYPE node_id;
            //! Transaction ID, indexes into tx_entry::progress
            PACKET_TX_ID_TYPE tx_id;
            //! Checksum of file. Combination of tx_id and file_crc to ideally be unique.
            PACKET_FILE_CRC_TYPE file_crc;
        };

        typedef uint8_t PACKET_QUEUE_FLAGS_TYPE;
        #define PACKET_QUEUE_FLAGS_LIMIT (PROGRESS_QUEUE_SIZE/(COSMOS_SIZEOF(PACKET_QUEUE_FLAGS_TYPE)*8))
        
        struct packet_struct_queue
        {
            file_packet_header header;
            //! An array of 32 uint8_t's, equaling 256 bits total, each corresponding to
            //! a tx_id in the outgoing/incoming queues.
            PACKET_QUEUE_FLAGS_TYPE tx_ids[PACKET_QUEUE_FLAGS_LIMIT];
        };

        using packet_struct_reqmeta = packet_struct_queue;

        struct packet_struct_metadata
        {
            file_packet_header header;
            uint8_t agent_name_len;
            string agent_name;
            uint8_t file_name_len;
            string file_name;
            PACKET_FILE_SIZE_TYPE file_size;
        };

        struct packet_struct_reqdata
        {
            file_packet_header header;
            uint16_t num_holes;
            vector<file_progress> holes;
        };

        struct packet_struct_data
        {
            file_packet_header header;
            PACKET_CHUNK_SIZE_TYPE byte_count;
            PACKET_FILE_SIZE_TYPE chunk_start;
            vector<PACKET_BYTE> chunk;
        };

        struct packet_struct_reqcomplete
        {
            file_packet_header header;
        };

        struct packet_struct_complete
        {
            file_packet_header header;
        };

        struct packet_struct_cancel
        {
            file_packet_header header;
        };

        struct packet_struct_metafile
        {
            uint8_t node_name_len;
            string node_name;
            PACKET_TX_ID_TYPE tx_id;
            PACKET_FILE_CRC_TYPE file_crc;
            uint8_t agent_name_len;
            string agent_name;
            uint8_t file_name_len;
            string file_name;
            PACKET_FILE_SIZE_TYPE file_size;
        };

        /// Holds data about the transfer progress of a single file.
        struct tx_progress
        {
            PACKET_TX_ID_TYPE tx_id=0;
            PACKET_FILE_CRC_TYPE file_crc=0;
            // Whether the file is marked for transfer
            bool enabled=false;
            // If initial METADATA has been sent/received
            bool sentmeta=false;
            // If all DATA has been sent/received
            bool sentdata=false;
            // For sender, set if COMPLETE packed has been received
            bool complete=false;
            string node_name="";
            string agent_name="";
            string file_name="";
            // Path to final file location
            string filepath="";
            // Path to temporary meta and incomplete data files
            string temppath="";
            // Time since last write_meta()
            ElapsedTime savetime;
            // Timer until ok to send out another response-type packet.
            ElapsedTime response_timer;
            // Size of the full file
            PACKET_FILE_SIZE_TYPE file_size=0;
            // Total bytes sent/received so far
            PACKET_FILE_SIZE_TYPE total_bytes=0;
            // Chunks to be sent, or chunks that have been received
            vector<file_progress> file_info;
            FILE * fp = nullptr;
        };

        /// Holds data about the queue of file transfers in progress.
        struct tx_entry
        {
            bool sentqueue = false;
            // Number of files in the incoming or outgoing queue
            PACKET_TX_ID_TYPE size;
            string node_name="";
            // Time to wait before sending out another response request or respond packet
            double waittime = 60.;
            // Vector of tx_id's needing responses. Used by the incoming queue.
            vector<PACKET_TX_ID_TYPE> respond;
            //! The incoming or outgoing queue. Indexed by the tx_id.
            //! tx_id of 0 holds the special semantic meaning of "not in use", so
            //! don't use progress[0].
            tx_progress progress[PROGRESS_QUEUE_SIZE];
        };

        /// Holds the incoming and outgoing queues for a single node.
        struct tx_queue
        {
            string node_name="";
            PACKET_NODE_ID_TYPE node_id;
            tx_entry incoming;
            tx_entry outgoing;
        };

        //Function which gets the size of a file
        int32_t get_file_size(string filename, PACKET_FILE_SIZE_TYPE& size);
        int32_t get_file_size(const char* filename, PACKET_FILE_SIZE_TYPE& size);

        // Converts packet types to and from byte arrays
        void serialize_queue(PacketComm& packet, PACKET_NODE_ID_TYPE node_id, const vector<PACKET_TX_ID_TYPE>& queue);
        int32_t deserialize_queue(const vector<PACKET_BYTE>& pdata, packet_struct_queue& queue);
        void serialize_cancel(PacketComm& packet, PACKET_NODE_ID_TYPE node_id, PACKET_TX_ID_TYPE tx_id, PACKET_FILE_CRC_TYPE file_crc);
        int32_t deserialize_cancel(const vector<PACKET_BYTE>& pdata, packet_struct_cancel& cancel);
        void serialize_reqcomplete(PacketComm& packet, PACKET_NODE_ID_TYPE node_id, PACKET_TX_ID_TYPE tx_id, PACKET_FILE_CRC_TYPE file_crc);
        int32_t deserialize_reqcomplete(const vector<PACKET_BYTE>& pdata, packet_struct_reqcomplete &reqcomplete);
        void serialize_complete(PacketComm& packet, PACKET_NODE_ID_TYPE node_id, PACKET_TX_ID_TYPE tx_id, PACKET_FILE_CRC_TYPE file_crc);
        int32_t deserialize_complete(const vector<PACKET_BYTE>& pdata, packet_struct_complete& complete);
        void serialize_reqmeta(PacketComm& packet, PACKET_NODE_ID_TYPE node_id, string node_name, const vector<PACKET_TX_ID_TYPE>& reqmeta);
        int32_t deserialize_reqmeta(const vector<PACKET_BYTE>& pdata, packet_struct_reqmeta& reqmeta);
        void serialize_reqdata(vector<PacketComm>& packets, PACKET_NODE_ID_TYPE self_node_id, PACKET_NODE_ID_TYPE orig_node_id, PACKET_TX_ID_TYPE tx_id, PACKET_FILE_CRC_TYPE file_crc, vector<file_progress>& holes, PACKET_CHUNK_SIZE_TYPE packet_data_size);
        int32_t deserialize_reqdata(const vector<PACKET_BYTE>& pdata, packet_struct_reqdata& reqdata);
        void serialize_metafile(vector<PACKET_BYTE>& pdata, PACKET_TX_ID_TYPE tx_id, PACKET_FILE_CRC_TYPE file_crc, const string& file_name, PACKET_FILE_SIZE_TYPE file_size, const string& node_name, const string& agent_name);
        int32_t deserialize_metafile(const vector<PACKET_BYTE>& pdata, packet_struct_metafile& meta);
        void serialize_metadata(PacketComm& packet, PACKET_NODE_ID_TYPE node_id, PACKET_TX_ID_TYPE tx_id, PACKET_FILE_CRC_TYPE file_crc, const string& file_name, PACKET_FILE_SIZE_TYPE file_size, const string& agent_name);
        int32_t deserialize_metadata(const vector<PACKET_BYTE>& pdata, packet_struct_metadata& meta);
        void serialize_data(PacketComm& packet, PACKET_NODE_ID_TYPE node_id, PACKET_TX_ID_TYPE tx_id, PACKET_FILE_CRC_TYPE file_crc, PACKET_CHUNK_SIZE_TYPE byte_count, PACKET_FILE_SIZE_TYPE chunk_start, PACKET_BYTE* chunk);
        int32_t deserialize_data(const vector<PACKET_BYTE>& pdata, packet_struct_data& data);

        // Accumulate and manage chunks
        PACKET_FILE_SIZE_TYPE merge_chunks_overlap(tx_progress& tx);
        vector<file_progress> find_chunks_missing(tx_progress& tx);
        bool add_chunk(tx_progress& tx, const file_progress& tp);
        bool add_chunk(tx_progress& tx, const file_progress& tp, bool first, bool last);
        bool add_chunks(tx_progress& tx, const vector<file_progress>& holes, uint8_t start_end_signifier);

        // Random utility functions
        bool filestruc_smaller_by_size(const filestruc& a, const filestruc& b);
        bool lower_chunk(file_progress i,file_progress j);
        int32_t clear_tx_entry(tx_entry& tx);
        int32_t flush_tx_entry(tx_entry& tx);
    }
}

#endif
