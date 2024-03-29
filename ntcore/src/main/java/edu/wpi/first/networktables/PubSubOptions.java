// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

package edu.wpi.first.networktables;

/** NetworkTables publish/subscribe options. */
@SuppressWarnings("MemberName")
public class PubSubOptions {
  /**
   * Construct from a list of options.
   *
   * @param options options
   */
  public PubSubOptions(PubSubOption... options) {
    for (PubSubOption option : options) {
      switch (option.m_kind) {
        case periodic:
          periodic = option.m_dValue;
          break;
        case sendAll:
          sendAll = option.m_bValue;
          break;
        case topicsOnly:
          topicsOnly = option.m_bValue;
          break;
        case pollStorage:
          pollStorage = option.m_iValue;
          break;
        case keepDuplicates:
          keepDuplicates = option.m_bValue;
          break;
        case disableRemote:
          disableRemote = option.m_bValue;
          break;
        case disableLocal:
          disableLocal = option.m_bValue;
          break;
        case excludePublisher:
          excludePublisher = option.m_iValue;
          break;
        case excludeSelf:
          excludeSelf = option.m_bValue;
          break;
        default:
          break;
      }
    }
  }

  PubSubOptions(
      int pollStorage,
      double periodic,
      int excludePublisher,
      boolean sendAll,
      boolean topicsOnly,
      boolean keepDuplicates,
      boolean prefixMatch,
      boolean disableRemote,
      boolean disableLocal,
      boolean excludeSelf) {
    this.pollStorage = pollStorage;
    this.periodic = periodic;
    this.excludePublisher = excludePublisher;
    this.sendAll = sendAll;
    this.topicsOnly = topicsOnly;
    this.keepDuplicates = keepDuplicates;
    this.prefixMatch = prefixMatch;
    this.disableRemote = disableRemote;
    this.disableLocal = disableLocal;
    this.excludeSelf = excludeSelf;
  }

  /** Default value of periodic. */
  public static final double kDefaultPeriodic = 0.1;

  /**
   * Polling storage size for a subscription. Specifies the maximum number of updates NetworkTables
   * should store between calls to the subscriber's readQueue() function. If zero, defaults to 1 if
   * sendAll is false, 20 if sendAll is true.
   */
  public int pollStorage;

  /**
   * How frequently changes will be sent over the network, in seconds. NetworkTables may send more
   * frequently than this (e.g. use a combined minimum period for all values) or apply a restricted
   * range to this value. The default is 100 ms.
   */
  public double periodic = kDefaultPeriodic;

  /**
   * For subscriptions, if non-zero, value updates for readQueue() are not queued for this
   * publisher.
   */
  public int excludePublisher;

  /** Send all value changes over the network. */
  public boolean sendAll;

  /** For subscriptions, don't ask for value changes (only topic announcements). */
  public boolean topicsOnly;

  /** Preserve duplicate value changes (rather than ignoring them). */
  public boolean keepDuplicates;

  /**
   * Perform prefix match on subscriber topic names. Is ignored/overridden by subscribe() functions;
   * only present in struct for the purposes of getting information about subscriptions.
   */
  public boolean prefixMatch;

  /**
   * For subscriptions, if remote value updates should not be queued for readQueue(). See also
   * disableLocal.
   */
  public boolean disableRemote;

  /**
   * For subscriptions, if local value updates should not be queued for readQueue(). See also
   * disableRemote.
   */
  public boolean disableLocal;

  /** For entries, don't queue (for readQueue) value updates for the entry's internal publisher. */
  public boolean excludeSelf;
}
