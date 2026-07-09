#include <gtest/gtest.h>

#include "ring_file_system.h"
#include "flashsim.h"

namespace {

struct slot_data_s {
	uint32_t ts;
	uint32_t id;
};

constexpr uint32_t kMaxSlots = DIST_MEM_SIZE / DIST_SLOT_SIZE;
constexpr uint32_t kMaxDataSlots = kMaxSlots - (EXT_MEM_SECTOR_SIZE / DIST_SLOT_SIZE);
constexpr uint32_t kTsBase = 3000000u;

void init_distance_em(void)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
	em_distance = em_driver_init_((void*)op_sector_erase,
			  (void*)op_read,
			  (void*)op_program,
			  (void*)load_index,
			  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
			DIST_SLOT_SIZE,
			DIST_START_ADDR);
	em_reset_(em_distance);
}

void write_slots(uint32_t count, uint32_t ts_start)
{
	slot_data_s data = {0};
	for (uint32_t i = 0; i < count; i++) {
		data.ts = ts_start + i;
		data.id = i;
		add_slot_(em_distance, (uint8_t*)&data);
	}
}

void read_slots(uint32_t count)
{
	slot_data_s read_data = {0};
	for (uint32_t i = 0; i < count; i++) {
		read_slot_(em_distance, (uint8_t*)&read_data);
	}
}

uint32_t oldest_ts(uint32_t total_writes)
{
	if (total_writes <= kMaxDataSlots) {
		return kTsBase;
	}
	return kTsBase + (total_writes - kMaxDataSlots);
}

} // namespace

TEST(test_pos_1, lots_read_2_slots) 
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);


	em_track = em_driver_init_((void*)op_sector_erase,
							  (void*)op_read,
							  (void*)op_program,
							  (void*)load_index,
							  (void*)save_index,
						      TRACK_MEM_SIZE,
					          EXT_MEM_SECTOR_SIZE,
				              TRACK_SLOT_SIZE,
				              TRACK_START_ADDR);



	em_reset_(em_track);

        struct position_s {
			uint32_t ut;
			float lon;
		    	float lat;
		    	uint32_t dummy;
		}data = {
				1687364137,
			       	37.5,
				55.6,
				0	
			};
	for (uint32_t i = 0; i < 2; i++) //Write 2 slots.
	{
		add_slot_(em_track, (uint8_t*)&data);
		data.ut++;
		
	}
	int32_t count = 0;

	struct position_s data_for_read = {0};

	for (uint32_t i = 0; i < 2; i++) //Read 2 slots
	{
		count = read_slot_(em_track, (uint8_t*)&data_for_read);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364137 + 1, data_for_read.ut);
	EXPECT_FLOAT_EQ(37.5, data_for_read.lon);
	EXPECT_FLOAT_EQ(55.6, data_for_read.lat);

	flashsim_close(sim);
}




TEST(test_1, lots_read_2_slots) 
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
	em_distance = em_driver_init_((void*)op_sector_erase,
			  (void*)op_read,
			  (void*)op_program,
			  (void*)load_index,
			  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364137;
	
	for (uint32_t i = 0; i < 2; i++) //Write 2 slots.
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 2; i++) //Read 2 slots
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364137 + 1, ts);
	flashsim_close(sim);
}


TEST(test_1, case_write_200_slots_read_100_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);

        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364138;
	
	for (uint32_t i = 0; i < 200; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 100; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(100, count);
	EXPECT_EQ(1687364138 + 99, ts);
	flashsim_close(sim);
}

TEST(test_1, case_write_150_slots_read_150_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364132;
	
	for (uint32_t i = 0; i < 150; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 150; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364132 + 149, ts);
	flashsim_close(sim);
}


TEST(test_1, case_write_511_slots_read_511_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364135;
	
	for (uint32_t i = 0; i < 511; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 511; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364135 + 510, ts);
	flashsim_close(sim);
}

TEST(test_1, case_write_512_slots_read_512_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364130;
	
	for (uint32_t i = 0; i < 512; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 512; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364130 + 511, ts);
	flashsim_close(sim);
}

TEST(test_1, case_write_512_slots_read_333_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364130;
	
	for (uint32_t i = 0; i < 512; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 333; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(179, count);
	EXPECT_EQ(1687364130 + 332, ts);
	flashsim_close(sim);
}



TEST(test_1, case_write_520_slots_read_520_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364123;
	
	for (uint32_t i = 0; i < 520; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 520; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364123 + 519, ts);
	flashsim_close(sim);

}

TEST(test_1, case_write_4096_slots_read_4096_slots)
{
	sim = flashsim_open("example.sim", 256*4096, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364147;
	
	for (uint32_t i = 0; i < 4096; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 4096; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364147 + 4095, ts);
	flashsim_close(sim);

}

TEST(test_1, case_write_4097_slots_read_4097)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364157;
	
	for (uint32_t i = 0; i < 4097; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	for (uint32_t i = 0; i < 4097; i++) 
	{
		count = read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(0, count);
	EXPECT_EQ(1687364157 + 4096, ts);
	flashsim_close(sim);

}


TEST(test_2_overwrite, case_write_69632_slots_read_1_slot )
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364197;
	
	for (uint32_t i = 0; i < 65536 + 4096; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364197 + 4608, ts);
	flashsim_close(sim);

}

TEST(test_2_overwrite, case_write_69633_slots_read_1_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364167;
	
	for (uint32_t i = 0; i < 65536 + 4097; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364167 + 4609, ts);
	flashsim_close(sim);

}

TEST(test_2_overwrite, case_write_65537_slots_read_1_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364167;
	
	for (uint32_t i = 0; i < 65536 + 1; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364167 + 513, ts);
	flashsim_close(sim);

}

TEST(test_2_overwrite, case_write_69636_slots_read_1_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364117;
	
	for (uint32_t i = 0; i < 65536 + 4100; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364117 + 4612, ts);
	flashsim_close(sim);

}

TEST(test_2_overwrite, case_write_69637_slots_read_1_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364107;
	
	for (uint32_t i = 0; i < 65536 + 4101; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364107 + 4613, ts);
	flashsim_close(sim);

}


TEST(test_2_overwrite, case_write_69638_read_1_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364237;
	
	for (uint32_t i = 0; i < 65536 + 4102; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364237 + 4614, ts);
	flashsim_close(sim);

}

TEST(test_2_overwrite, case_write_71536_read_1_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);


	uint16_t j = 0;
	uint32_t t = 1687364437;
	
	for (uint32_t i = 0; i < 65536 + 6000; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364437 + 6512, ts);
	flashsim_close(sim);

}

TEST(test_2_overwrite, case_write_71556_slots_read_1_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364737;
	
	for (uint32_t i = 0; i < 65536 + 6020; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	EXPECT_EQ(1687364737 + 6532, ts);
	flashsim_close(sim);

}

TEST(test_3_overwrite_buffer, case_write_200_read_50_write_320_read_1)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687364037;
	
	for (uint32_t i = 0; i < 200; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}

	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;

	for (uint32_t i =0; i<50; i++)
	{
		count = read_distance_data(&distance, &ts);
	}

	for (uint32_t i = 0; i < 320; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
		count = read_distance_data(&distance, &ts);

	EXPECT_EQ(1687364037 + 50, ts);
	flashsim_close(sim);

}

TEST(test_3_overwrite_buffer, case_write_812_read_100_write_612_read_1)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687368137;
	
	for (uint32_t i = 0; i < 812; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}

	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;

	for (uint32_t i =0; i<612; i++)
	{
		count = read_distance_data(&distance, &ts);
	}

	for (uint32_t i = 0; i < 262; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
		count = read_distance_data(&distance, &ts);

	EXPECT_EQ(1687368137 + 612, ts);
	flashsim_close(sim);

}

TEST(test_3_overwrite_buffer, case_write_511_read_1_write_2_read_1)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687368137;
	
	for (uint32_t i = 0; i < 511; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}

	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;

	count =  read_distance_data(&distance, &ts);
	for (uint32_t i = 0; i < 2; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(1687368137 + 1, ts);
	flashsim_close(sim);
}

TEST(test_3_overwrite_read_overwrite, case_write_511_read_1_write_2_read_1)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);

	uint16_t j = 0;
	uint32_t t = 1687368137;
	
	for (uint32_t i = 0; i < 512 + 256 + 128; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}

	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;

	for (uint32_t i = 0; i < 512 + 256; i++) 
	{
		count =  read_distance_data(&distance, &ts);
	}
	EXPECT_EQ(1687368137 + 512 + 255, ts);	
	for (uint32_t i = 0; i < 65536 - 256 - 128; i++) 
	{
		add_distance_data(100 + j, t++);
		j+=3;
	}

	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(1687368137 + 1024, ts);
	flashsim_close(sim);
}


TEST(test_4_count, case_write_some_slote_read_one)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
        em_distance = em_driver_init_((void*)op_sector_erase,
				  (void*)op_read,
				  (void*)op_program,
				  (void*)load_index,
				  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
                        DIST_SLOT_SIZE,
                        DIST_START_ADDR);
	em_reset_(em_distance);
	uint16_t j = 0;
	uint32_t t = 1687364139;
	for (uint32_t i = 0; i < 65536; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	uint16_t distance = 0;
       	uint32_t ts = 0;
	uint32_t count = 0;

	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);
	
	j = 0;
	for (uint32_t i = 0; i < 143; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);

	j = 0;
	for (uint32_t i = 0; i < 512; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);

	j = 0;
	for (uint32_t i = 0; i < 512; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);

	j = 0;
	for (uint32_t i = 0; i < 512; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);

	j = 0;
	for (uint32_t i = 0; i < 512*8; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);

	j = 0;
	for (uint32_t i = 0; i < 300; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);

	j = 0;
	for (uint32_t i = 0; i < 69; i++) 
	{
		add_distance_data(100 + j, ++t);
		j+=3;
	}
	
	count = read_distance_data(&distance, &ts);
	EXPECT_EQ(65023, count);

	for (uint32_t i = 0; i < 504; i++)
	{
		count = read_distance_data(&distance, &ts);
	}

	EXPECT_EQ(64519, count);
	flashsim_close(sim);
}

TEST(test_5_recover_discard, recover_one_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
	em_distance = em_driver_init_((void*)op_sector_erase,
			  (void*)op_read,
			  (void*)op_program,
			  (void*)load_index,
			  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
			DIST_SLOT_SIZE,
			DIST_START_ADDR);
	em_reset_(em_distance);

	struct slot_data_s {
		uint32_t ts;
		uint32_t id;
	} data = {0};

	for (uint32_t i = 0; i < 3; i++) {
		data.ts = 1000 + i;
		data.id = i;
		add_slot_(em_distance, (uint8_t*)&data);
	}

	struct slot_data_s read_data = {0};
	read_slot_(em_distance, (uint8_t*)&read_data);
	EXPECT_EQ(1000u, read_data.ts);
	EXPECT_EQ(2, get_slot_count_(em_distance));

	EXPECT_EQ(3, recover_slot_(em_distance));
	EXPECT_EQ(3, get_slot_count_(em_distance));
	EXPECT_EQ(-1, recover_slot_(em_distance));

	read_slot_(em_distance, (uint8_t*)&read_data);
	EXPECT_EQ(1000u, read_data.ts);
	EXPECT_EQ(2, get_slot_count_(em_distance));

	flashsim_close(sim);
}

TEST(test_5_recover_discard, discard_one_slot)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
	em_distance = em_driver_init_((void*)op_sector_erase,
			  (void*)op_read,
			  (void*)op_program,
			  (void*)load_index,
			  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
			DIST_SLOT_SIZE,
			DIST_START_ADDR);
	em_reset_(em_distance);

	struct slot_data_s {
		uint32_t ts;
		uint32_t id;
	} data = {0};

	for (uint32_t i = 0; i < 3; i++) {
		data.ts = 2000 + i;
		data.id = i;
		add_slot_(em_distance, (uint8_t*)&data);
	}

	struct slot_data_s read_data = {0};
	for (uint32_t i = 0; i < 2; i++) {
		read_slot_(em_distance, (uint8_t*)&read_data);
	}
	EXPECT_EQ(2001u, read_data.ts);
	EXPECT_EQ(1, get_slot_count_(em_distance));

	EXPECT_EQ(1, discard_slot_(em_distance));
	EXPECT_EQ(2, recover_slot_(em_distance));
	EXPECT_EQ(2, get_slot_count_(em_distance));

	read_slot_(em_distance, (uint8_t*)&read_data);
	EXPECT_EQ(2001u, read_data.ts);

	EXPECT_EQ(1, discard_slot_(em_distance));
	EXPECT_EQ(-1, recover_slot_(em_distance));
	EXPECT_EQ(-1, discard_slot_(em_distance));

	flashsim_close(sim);
}

TEST(test_5_recover_discard, recover_all_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
	em_distance = em_driver_init_((void*)op_sector_erase,
			  (void*)op_read,
			  (void*)op_program,
			  (void*)load_index,
			  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
			DIST_SLOT_SIZE,
			DIST_START_ADDR);
	em_reset_(em_distance);

	struct slot_data_s {
		uint32_t ts;
		uint32_t id;
	} data = {0};

	for (uint32_t i = 0; i < 5; i++) {
		data.ts = 3000 + i;
		data.id = i;
		add_slot_(em_distance, (uint8_t*)&data);
	}

	struct slot_data_s read_data = {0};
	for (uint32_t i = 0; i < 3; i++) {
		read_slot_(em_distance, (uint8_t*)&read_data);
	}
	EXPECT_EQ(3002u, read_data.ts);
	EXPECT_EQ(2, get_slot_count_(em_distance));

	EXPECT_EQ(5, recover_all_slots_(em_distance));
	EXPECT_EQ(5, get_slot_count_(em_distance));

	for (uint32_t i = 0; i < 5; i++) {
		EXPECT_EQ(4 - i, read_slot_(em_distance, (uint8_t*)&read_data));
		EXPECT_EQ(3000u + i, read_data.ts);
	}
	EXPECT_EQ(-1, read_slot_(em_distance, (uint8_t*)&read_data));

	flashsim_close(sim);
}

TEST(test_5_recover_discard, discard_all_slots)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
	em_distance = em_driver_init_((void*)op_sector_erase,
			  (void*)op_read,
			  (void*)op_program,
			  (void*)load_index,
			  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
			DIST_SLOT_SIZE,
			DIST_START_ADDR);
	em_reset_(em_distance);

	struct slot_data_s {
		uint32_t ts;
		uint32_t id;
	} data = {0};

	for (uint32_t i = 0; i < 5; i++) {
		data.ts = 4000 + i;
		data.id = i;
		add_slot_(em_distance, (uint8_t*)&data);
	}

	struct slot_data_s read_data = {0};
	for (uint32_t i = 0; i < 3; i++) {
		read_slot_(em_distance, (uint8_t*)&read_data);
	}
	EXPECT_EQ(4002u, read_data.ts);
	EXPECT_EQ(2, get_slot_count_(em_distance));

	EXPECT_EQ(0, discard_all_slots_(em_distance));
	EXPECT_EQ(0, get_slot_count_(em_distance));
	EXPECT_EQ(-1, recover_slot_(em_distance));
	EXPECT_EQ(-1, discard_slot_(em_distance));
	EXPECT_EQ(-1, read_slot_(em_distance, (uint8_t*)&read_data));

	flashsim_close(sim);
}

TEST(test_5_recover_discard, no_op_without_reads)
{
	sim = flashsim_open("example.sim", FLASHSIM_SIZE, 4096);
	em_distance = em_driver_init_((void*)op_sector_erase,
			  (void*)op_read,
			  (void*)op_program,
			  (void*)load_index,
			  (void*)save_index,
			DIST_MEM_SIZE,
			EXT_MEM_SECTOR_SIZE,
			DIST_SLOT_SIZE,
			DIST_START_ADDR);
	em_reset_(em_distance);

	struct slot_data_s {
		uint32_t ts;
		uint32_t id;
	} data = {3000, 0};

	add_slot_(em_distance, (uint8_t*)&data);
	EXPECT_EQ(1, get_slot_count_(em_distance));
	EXPECT_EQ(-1, recover_slot_(em_distance));
	EXPECT_EQ(-1, discard_slot_(em_distance));
	EXPECT_EQ(1, recover_all_slots_(em_distance));
	EXPECT_EQ(1, get_slot_count_(em_distance));
	EXPECT_EQ(0, discard_all_slots_(em_distance));
	EXPECT_EQ(0, get_slot_count_(em_distance));

	flashsim_close(sim);
}

TEST(test_6_overflow_discard, discard_all_after_overflow_read)
{
	const uint32_t overflow = 600;
	const uint32_t total_writes = kMaxDataSlots + overflow;

	init_distance_em();
	write_slots(total_writes, kTsBase);
	EXPECT_EQ(kMaxDataSlots, get_slot_count_(em_distance));

	read_slots(500);
	EXPECT_EQ(kMaxDataSlots - 500, get_slot_count_(em_distance));

	EXPECT_EQ(0, discard_all_slots_(em_distance));
	EXPECT_EQ(0, get_slot_count_(em_distance));
	EXPECT_EQ(-1, recover_slot_(em_distance));
	EXPECT_EQ(-1, discard_slot_(em_distance));

	slot_data_s read_data = {0};
	EXPECT_EQ(-1, read_slot_(em_distance, (uint8_t*)&read_data));

	flashsim_close(sim);
}

TEST(test_6_overflow_discard, discard_blocks_recover_and_reads_next_unread)
{
	const uint32_t overflow = 600;
	const uint32_t total_writes = kMaxDataSlots + overflow;
	const uint32_t read_count = 200;

	init_distance_em();
	write_slots(total_writes, kTsBase);
	read_slots(read_count);

	for (uint32_t i = 0; i < read_count; i++) {
		EXPECT_EQ(kMaxDataSlots - read_count, discard_slot_(em_distance));
	}

	EXPECT_EQ(-1, recover_slot_(em_distance));

	slot_data_s read_data = {0};
	EXPECT_EQ(kMaxDataSlots - read_count - 1,
		  read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes) + read_count, read_data.ts);

	flashsim_close(sim);
}

TEST(test_6_overflow_recover, recover_single_at_overflow_boundary)
{
	const uint32_t total_writes = kMaxDataSlots + 1;

	init_distance_em();
	write_slots(total_writes, kTsBase);

	slot_data_s read_data = {0};
	EXPECT_EQ(kMaxDataSlots - 1, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes) + 1, read_data.ts);

	EXPECT_EQ(kMaxDataSlots, recover_slot_(em_distance));
	EXPECT_EQ(-1, recover_slot_(em_distance));

	EXPECT_EQ(kMaxDataSlots - 1, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes) + 1, read_data.ts);

	flashsim_close(sim);
}

TEST(test_6_overflow_recover, recover_all_after_overflow_read)
{
	const uint32_t overflow = 1000;
	const uint32_t total_writes = kMaxDataSlots + overflow;
	const uint32_t read_count = 500;

	init_distance_em();
	write_slots(total_writes, kTsBase);
	read_slots(read_count);

	EXPECT_EQ(kMaxDataSlots, recover_all_slots_(em_distance));
	EXPECT_EQ(kMaxDataSlots, get_slot_count_(em_distance));

	slot_data_s read_data = {0};
	EXPECT_EQ(kMaxDataSlots - 1, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes), read_data.ts);

	for (uint32_t i = 1; i < kMaxDataSlots; i++) {
		read_slot_(em_distance, (uint8_t*)&read_data);
	}
	EXPECT_EQ(0, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes) + kMaxDataSlots - 1, read_data.ts);

	flashsim_close(sim);
}

TEST(test_6_overflow_recover, recover_at_sector_boundary)
{
	const uint32_t overflow = EXT_MEM_SECTOR_SIZE / DIST_SLOT_SIZE;
	const uint32_t total_writes = kMaxDataSlots + overflow;

	init_distance_em();
	write_slots(total_writes, kTsBase);

	slot_data_s read_data = {0};
	EXPECT_EQ(kMaxDataSlots - 1, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes), read_data.ts);

	EXPECT_EQ(kMaxDataSlots, recover_slot_(em_distance));
	EXPECT_EQ(kMaxDataSlots - 1, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes), read_data.ts);

	flashsim_close(sim);
}

TEST(test_6_overflow_recover, recover_all_after_full_index_wrap)
{
	const uint32_t overflow = 100;
	const uint32_t total_writes = kMaxSlots + kMaxDataSlots + overflow;

	init_distance_em();
	write_slots(total_writes, kTsBase);
	read_slots(50);

	EXPECT_EQ(kMaxDataSlots, recover_all_slots_(em_distance));
	EXPECT_EQ(kMaxDataSlots, get_slot_count_(em_distance));

	slot_data_s read_data = {0};
	EXPECT_EQ(kMaxDataSlots - 1, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(oldest_ts(total_writes), read_data.ts);

	flashsim_close(sim);
}

TEST(test_6_overflow_combo, recover_discard_interleaved)
{
	const uint32_t overflow = 800;
	const uint32_t total_writes = kMaxDataSlots + overflow;
	const uint32_t oldest = total_writes - kMaxDataSlots;

	init_distance_em();
	write_slots(total_writes, kTsBase);
	read_slots(300);

	for (uint32_t i = 0; i < 100; i++) {
		discard_slot_(em_distance);
	}
	for (uint32_t i = 0; i < 50; i++) {
		EXPECT_EQ(kMaxDataSlots - 300 + i + 1, recover_slot_(em_distance));
	}

	slot_data_s read_data = {0};
	EXPECT_EQ(kMaxDataSlots - 251,
		  read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(kTsBase + oldest + 250, read_data.ts);

	for (uint32_t i = 0; i < 151; i++) {
		discard_slot_(em_distance);
	}
	EXPECT_EQ(-1, recover_slot_(em_distance));

	for (uint32_t i = 0; i < 49; i++) {
		read_slot_(em_distance, (uint8_t*)&read_data);
	}
	EXPECT_EQ(kMaxDataSlots - 301,
		  read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(kTsBase + oldest + 300, read_data.ts);

	flashsim_close(sim);
}

TEST(test_6_overflow_combo, recover_all_then_discard_all)
{
	const uint32_t overflow = 600;
	const uint32_t total_writes = kMaxDataSlots + overflow;

	init_distance_em();
	write_slots(total_writes, kTsBase);
	read_slots(100);

	EXPECT_EQ(kMaxDataSlots, recover_all_slots_(em_distance));
	EXPECT_EQ(kMaxDataSlots, get_slot_count_(em_distance));

	EXPECT_EQ(0, discard_all_slots_(em_distance));
	EXPECT_EQ(0, get_slot_count_(em_distance));
	EXPECT_EQ(-1, recover_slot_(em_distance));

	slot_data_s read_data = {0};
	EXPECT_EQ(-1, read_slot_(em_distance, (uint8_t*)&read_data));

	flashsim_close(sim);
}

TEST(test_6_overflow_combo, write_after_discard_all_and_reset)
{
	const uint32_t overflow = 500;
	const uint32_t total_writes = kMaxDataSlots + overflow;
	const uint32_t rewrite_ts = kTsBase + total_writes;

	init_distance_em();
	write_slots(total_writes, kTsBase);
	read_slots(400);
	EXPECT_EQ(0, discard_all_slots_(em_distance));
	EXPECT_EQ(0, get_slot_count_(em_distance));

	em_reset_(em_distance);
	write_slots(20, rewrite_ts);

	EXPECT_EQ(20, get_slot_count_(em_distance));

	slot_data_s read_data = {0};
	EXPECT_EQ(19, read_slot_(em_distance, (uint8_t*)&read_data));
	EXPECT_EQ(rewrite_ts, read_data.ts);

	for (uint32_t i = 1; i < 20; i++) {
		EXPECT_EQ(19 - i, read_slot_(em_distance, (uint8_t*)&read_data));
		EXPECT_EQ(rewrite_ts + i, read_data.ts);
	}

	flashsim_close(sim);
}

