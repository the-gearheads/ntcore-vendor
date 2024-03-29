// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

package edu.wpi.first.util.datalog;

/** Log raw byte array values. */
public class RawLogEntry extends DataLogEntry {
  public static final String kDataType = "raw";

  public RawLogEntry(DataLog log, String name, String metadata, String type, long timestamp) {
    super(log, name, type, metadata, timestamp);
  }

  public RawLogEntry(DataLog log, String name, String metadata, String type) {
    this(log, name, metadata, type, 0);
  }

  public RawLogEntry(DataLog log, String name, String metadata, long timestamp) {
    this(log, name, metadata, kDataType, timestamp);
  }

  public RawLogEntry(DataLog log, String name, String metadata) {
    this(log, name, metadata, 0);
  }

  public RawLogEntry(DataLog log, String name, long timestamp) {
    this(log, name, "", timestamp);
  }

  public RawLogEntry(DataLog log, String name) {
    this(log, name, 0);
  }

  /**
   * Appends a record to the log.
   *
   * @param value Value to record
   * @param timestamp Time stamp (0 to indicate now)
   */
  public void append(byte[] value, long timestamp) {
    m_log.appendRaw(m_entry, value, timestamp);
  }

  /**
   * Appends a record to the log.
   *
   * @param value Value to record
   */
  public void append(byte[] value) {
    m_log.appendRaw(m_entry, value, 0);
  }
}
