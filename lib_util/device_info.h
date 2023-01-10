#pragma once
#include <stdint.h>

class DeviceInfo
{
public:
	static void init(uint32_t sensor_id, 
				uint32_t sensor_instance_id)
	{
		m_sensor_id = sensor_id;
		m_sensor_instance_id = sensor_instance_id;

	}
	static uint32_t sensor_id() 
	{ 
		return m_sensor_id;
	}

	static uint32_t sensor_instance_id() 
	{ 
		return m_sensor_instance_id;
	}

private:
	static uint32_t m_sensor_id;
	static uint32_t m_sensor_instance_id;
};