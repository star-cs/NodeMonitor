CREATE DATABASE IF NOT EXISTS monitor_db DEFAULT CHARACTER SET utf8mb4;
USE monitor_db;

CREATE TABLE IF NOT EXISTS server_performance (
    id INT AUTO_INCREMENT PRIMARY KEY,
    server_name VARCHAR(255) NOT NULL,
    -- 内存
    total FLOAT,
    free FLOAT,
    avail FLOAT,
    send_rate FLOAT,
    rcv_rate FLOAT,
    score FLOAT,
    -- CPU
    cpu_percent FLOAT,
    usr_percent FLOAT,
    system_percent FLOAT,
    nice_percent FLOAT,
    idle_percent FLOAT,
    io_wait_percent FLOAT,
    irq_percent FLOAT,
    soft_irq_percent FLOAT,
    load_avg_1 FLOAT,
    load_avg_3 FLOAT,
    load_avg_15 FLOAT,
    mem_used_percent FLOAT,
    -- 变化率
    mem_used_percent_rate FLOAT,
    total_rate FLOAT,
    free_rate FLOAT,
    avail_rate FLOAT,
    send_rate_rate FLOAT,
    rcv_rate_rate FLOAT,
    cpu_percent_rate FLOAT,
    usr_percent_rate FLOAT,
    system_percent_rate FLOAT,
    nice_percent_rate FLOAT,
    idle_percent_rate FLOAT,
    io_wait_percent_rate FLOAT,
    irq_percent_rate FLOAT,
    soft_irq_percent_rate FLOAT,
    load_avg_1_rate FLOAT,
    load_avg_3_rate FLOAT,
    load_avg_15_rate FLOAT,
    -- 时间
    timestamp DATETIME,
    INDEX idx_server_time(server_name, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    buffers_rate FLOAT,
    cached_rate FLOAT,
    swap_cached_rate FLOAT,
    active_rate FLOAT,
    inactive_rate FLOAT,
    active_anon_rate FLOAT,
    inactive_anon_rate FLOAT,
    active_file_rate FLOAT,
    inactive_file_rate FLOAT,
    dirty_rate FLOAT,
    writeback_rate FLOAT,
    anon_pages_rate FLOAT,
    mapped_rate FLOAT,
    kreclaimable_rate FLOAT,
    sreclaimable_rate FLOAT,
    sunreclaim_rate FLOAT,
    timestamp DATETIME,
    INDEX idx_server_time(server_name, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 2. 网络详细数据表（删除 in_peak/out_peak 及其变化率字段）
CREATE TABLE IF NOT EXISTS server_net_detail (
    id INT AUTO_INCREMENT PRIMARY KEY,
    server_name VARCHAR(255) NOT NULL,
    net_name VARCHAR(64) NOT NULL,
    rcv_bytes_rate FLOAT DEFAULT 0,         -- (KB)/s
    rcv_packets_rate FLOAT DEFAULT 0,       -- (个)/s
    snd_bytes_rate FLOAT DEFAULT 0,         -- (KB)/s
    snd_packets_rate FLOAT DEFAULT 0,       -- (个)/s
    rcv_bytes_rate_rate FLOAT DEFAULT 0,    -- (KB/s)/s
    rcv_packets_rate_rate FLOAT DEFAULT 0,  -- (个/s)/s
    snd_bytes_rate_rate FLOAT DEFAULT 0,    -- (KB/s)/s
    snd_packets_rate_rate FLOAT DEFAULT 0,  -- (个/s)/s
    timestamp DATETIME NOT NULL,
    INDEX idx_server_time(server_name, net_name, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 3. 软中断详细数据表（每个字段增加变化率字段）
CREATE TABLE IF NOT EXISTS server_softirq_detail (
    id INT AUTO_INCREMENT PRIMARY KEY,
    server_name VARCHAR(255) NOT NULL,
    cpu_name VARCHAR(64) NOT NULL,
    hi BIGINT DEFAULT 0,
    timer BIGINT DEFAULT 0,
    net_tx BIGINT DEFAULT 0,
    net_rx BIGINT DEFAULT 0,
    block BIGINT DEFAULT 0,
    irq_poll BIGINT DEFAULT 0,
    tasklet BIGINT DEFAULT 0,
    sched BIGINT DEFAULT 0,
    hrtimer BIGINT DEFAULT 0,
    rcu BIGINT DEFAULT 0,
    hi_rate FLOAT DEFAULT 0,
    timer_rate FLOAT DEFAULT 0,
    net_tx_rate FLOAT DEFAULT 0,
    net_rx_rate FLOAT DEFAULT 0,
    block_rate FLOAT DEFAULT 0,
    irq_poll_rate FLOAT DEFAULT 0,
    tasklet_rate FLOAT DEFAULT 0,
    sched_rate FLOAT DEFAULT 0,
    hrtimer_rate FLOAT DEFAULT 0,
    rcu_rate FLOAT DEFAULT 0,
    timestamp DATETIME NOT NULL,
    INDEX idx_server_cpu_time(server_name, cpu_name, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 4. 内存详细数据表（删除 swap_used/swap_total/commit/commit_limit 及其变化率字段）
CREATE TABLE IF NOT EXISTS server_mem_detail (
    id INT AUTO_INCREMENT PRIMARY KEY,
    server_name VARCHAR(255) NOT NULL,
    total FLOAT DEFAULT 0,
    free FLOAT DEFAULT 0,
    avail FLOAT DEFAULT 0,
    buffers FLOAT DEFAULT 0,
    cached FLOAT DEFAULT 0,
    swap_cached FLOAT DEFAULT 0,
    active FLOAT DEFAULT 0,
    inactive FLOAT DEFAULT 0,
    active_anon FLOAT DEFAULT 0,
    inactive_anon FLOAT DEFAULT 0,
    active_file FLOAT DEFAULT 0,
    inactive_file FLOAT DEFAULT 0,
    dirty FLOAT DEFAULT 0,
    writeback FLOAT DEFAULT 0,
    anon_pages FLOAT DEFAULT 0,
    mapped FLOAT DEFAULT 0,
    kreclaimable FLOAT DEFAULT 0,
    sreclaimable FLOAT DEFAULT 0,
    sunreclaim FLOAT DEFAULT 0,
    total_rate FLOAT DEFAULT 0,
    free_rate FLOAT DEFAULT 0,
    avail_rate FLOAT DEFAULT 0,
    buffers_rate FLOAT DEFAULT 0,
    cached_rate FLOAT DEFAULT 0,
    swap_cached_rate FLOAT DEFAULT 0,
    active_rate FLOAT DEFAULT 0,
    inactive_rate FLOAT DEFAULT 0,
    active_anon_rate FLOAT DEFAULT 0,
    inactive_anon_rate FLOAT DEFAULT 0,
    active_file_rate FLOAT DEFAULT 0,
    inactive_file_rate FLOAT DEFAULT 0,
    dirty_rate FLOAT DEFAULT 0,
    writeback_rate FLOAT DEFAULT 0,
    anon_pages_rate FLOAT DEFAULT 0,
    mapped_rate FLOAT DEFAULT 0,
    kreclaimable_rate FLOAT DEFAULT 0,
    sreclaimable_rate FLOAT DEFAULT 0,
    sunreclaim_rate FLOAT DEFAULT 0,
    timestamp DATETIME NOT NULL,
    INDEX idx_server_time(server_name, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    INDEX idx_mem_used(total, free, avail)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    sreclaimable_rate FLOAT,
    sunreclaim_rate FLOAT,
    commit_rate FLOAT,
    commit_limit_rate FLOAT,
    timestamp DATETIME,
    INDEX idx_server_time(server_name, timestamp),
    INDEX idx_mem_used(total, free, avail),
    INDEX idx_swap_used(swap_used),
    INDEX idx_commit(commit)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    INDEX idx_mem_used(total, free, avail),
    INDEX idx_swap_used(swap_used),
    INDEX idx_commit(commit)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    commit_rate FLOAT,
    commit_limit_rate FLOAT,
    timestamp DATETIME,
    INDEX idx_server_time(server_name, timestamp),
    INDEX idx_mem_used(total, free, avail),
    INDEX idx_swap_used(swap_used),
    INDEX idx_commit(commit)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    sunreclaim_rate FLOAT,
    commit_rate FLOAT,
    commit_limit_rate FLOAT,
    timestamp DATETIME,
    INDEX idx_server_time(server_name, timestamp),
    INDEX idx_mem_used(total, free, avail),
    INDEX idx_swap_used(swap_used),
    INDEX idx_commit(commit)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    INDEX idx_mem_used(total, free, avail),
    INDEX idx_swap_used(swap_used),
    INDEX idx_commit(commit)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    commit_rate FLOAT,
    commit_limit_rate FLOAT,
    timestamp DATETIME,
    INDEX idx_server_time(server_name, timestamp),
    INDEX idx_mem_used(total, free, avail),
    INDEX idx_swap_used(swap_used),
    INDEX idx_commit(commit)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    INDEX idx_server_time(server_name, timestamp),
    INDEX idx_mem_used(total, free, avail),
    INDEX idx_swap_used(swap_used),
    INDEX idx_commit(commit)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS server_disk_detail (
    id INT AUTO_INCREMENT PRIMARY KEY,
    server_name VARCHAR(255) NOT NULL,
    disk_name VARCHAR(64) NOT NULL,
    reads BIGINT DEFAULT 0,
    writes BIGINT DEFAULT 0,
    sectors_read BIGINT DEFAULT 0,
    sectors_written BIGINT DEFAULT 0,
    read_time_ms BIGINT DEFAULT 0,
    write_time_ms BIGINT DEFAULT 0,
    io_in_progress BIGINT DEFAULT 0,
    io_time_ms BIGINT DEFAULT 0,
    weighted_io_time_ms BIGINT DEFAULT 0,
    read_bytes_per_sec FLOAT DEFAULT 0,
    write_bytes_per_sec FLOAT DEFAULT 0,
    read_iops FLOAT DEFAULT 0,
    write_iops FLOAT DEFAULT 0,
    avg_read_latency_ms FLOAT DEFAULT 0,
    avg_write_latency_ms FLOAT DEFAULT 0,
    util_percent FLOAT DEFAULT 0,
    -- 变化率字段
    read_bytes_per_sec_rate FLOAT DEFAULT 0,
    write_bytes_per_sec_rate FLOAT DEFAULT 0,
    read_iops_rate FLOAT DEFAULT 0,
    write_iops_rate FLOAT DEFAULT 0,
    avg_read_latency_ms_rate FLOAT DEFAULT 0,
    avg_write_latency_ms_rate FLOAT DEFAULT 0,
    util_percent_rate FLOAT DEFAULT 0,
    timestamp DATETIME NOT NULL,
    INDEX idx_server_disk_time(server_name, disk_name, timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
