/* net.c */

#include "utils/lwiplib.h"
#include "utils/ptpdlib.h"
#include "lwip/inet.h"

/* Network Buffer Queue Functions. */
static void netQInit(BufQueue *pQ)
{
  pQ->get = 0;
  pQ->put = 0;
  pQ->count = 0;
}

static Integer32 netQPut(BufQueue *pQ, void *pbuf)
{
  if(pQ->count >= PBUF_QUEUE_SIZE)
    return FALSE;
  pQ->pbuf[pQ->put] = pbuf;
  pQ->put = (pQ->put + 1) % PBUF_QUEUE_SIZE;
  pQ->count++;
  return TRUE;
}

void *netQGet(BufQueue *pQ)
{
  void *pbuf;

  if(!pQ->count)
    return NULL;
  pbuf = pQ->pbuf[pQ->get];
  pQ->get = (pQ->get + 1) % PBUF_QUEUE_SIZE;
  pQ->count--;
  return pbuf;
}

static Integer32 netQCheck(BufQueue *pQ)
{
  if(!pQ->count)
    return FALSE;
  return TRUE;
}

/* Processing an incoming message on the Event port. */
static void eventRecv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
  struct ip_addr *addr, u16_t port)
{
  NetPath *netPath = (NetPath *)arg;

  /* prevent warnings about unused arguments */
  (void)pcb; (void)addr; (void)port;

  /* Place the incoming message on the Event Port QUEUE. */
  if(!netQPut(&netPath->eventQ, p))
    PERROR("Event Queue Full!\n");
}

/* Processing an incoming message on the Event port. */
static void generalRecv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
  struct ip_addr *addr, u16_t port)
{
  NetPath *netPath = (NetPath *)arg;

  /* prevent warnings about unused arguments */
  (void)pcb; (void)addr; (void)port;

  /* Place the incoming message on the Event Port QUEUE. */
    if(!netQPut(&netPath->generalQ, p))
    {
        PERROR("Event Queue Full!\n");
    }
}

Boolean lookupSubdomainAddress(Octet *subdomainName, Octet *subdomainAddress)
{
  UInteger32 h;

  /* set multicast group address based on subdomainName */
  if (!memcmp(subdomainName, DEFAULT_PTP_DOMAIN_NAME, PTP_SUBDOMAIN_NAME_LENGTH))
    memcpy(subdomainAddress, DEFAULT_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);
  else if(!memcmp(subdomainName, ALTERNATE_PTP_DOMAIN1_NAME, PTP_SUBDOMAIN_NAME_LENGTH))
    memcpy(subdomainAddress, ALTERNATE_PTP_DOMAIN1_ADDRESS, NET_ADDRESS_LENGTH);
  else if(!memcmp(subdomainName, ALTERNATE_PTP_DOMAIN2_NAME, PTP_SUBDOMAIN_NAME_LENGTH))
    memcpy(subdomainAddress, ALTERNATE_PTP_DOMAIN2_ADDRESS, NET_ADDRESS_LENGTH);
  else if(!memcmp(subdomainName, ALTERNATE_PTP_DOMAIN3_NAME, PTP_SUBDOMAIN_NAME_LENGTH))
    memcpy(subdomainAddress, ALTERNATE_PTP_DOMAIN3_ADDRESS, NET_ADDRESS_LENGTH);
  else
  {
    h = crc_algorithm(subdomainName, PTP_SUBDOMAIN_NAME_LENGTH) % 3;
    switch(h)
    {
    case 0:
      memcpy(subdomainAddress, ALTERNATE_PTP_DOMAIN1_ADDRESS, NET_ADDRESS_LENGTH);
      break;
    case 1:
      memcpy(subdomainAddress, ALTERNATE_PTP_DOMAIN2_ADDRESS, NET_ADDRESS_LENGTH);
      break;
    case 2:
      memcpy(subdomainAddress, ALTERNATE_PTP_DOMAIN3_ADDRESS, NET_ADDRESS_LENGTH);
      break;
    default:
      ERROR("handle out of range for '%s'!\n", subdomainName);
      return FALSE;
    }
  }

  return TRUE;
}

Boolean netInit(NetPath *netPath, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
  int i;
  struct in_addr netAddr;
  char addrStr[NET_ADDRESS_LENGTH];

  DBG("netInit\n");

  /* Allocate tx buffer for the event port. */
  netPath->eventTxBuf = pbuf_alloc(PBUF_TRANSPORT, PACKET_SIZE, PBUF_RAM);
  if(netPath->eventTxBuf == NULL)
  {
    PERROR("Failed to allocate Event Tx Buffer\n");
    return FALSE;
  }

  /* Allocate tx buffer for the general port. */
  netPath->generalTxBuf = pbuf_alloc(PBUF_TRANSPORT, PACKET_SIZE, PBUF_RAM);
  if(netPath->generalTxBuf == NULL)
  {
    PERROR("Failed to allocate Event Tx Buffer\n");
    pbuf_free(netPath->eventTxBuf);
    return FALSE;
  }

  /* Open lwIP raw udp interfaces for the event port. */
  netPath->eventPcb = udp_new();
  if(netPath->eventPcb == NULL)
  {
    PERROR("Failed to open Event UDP PCB\n");
    pbuf_free(netPath->eventTxBuf);
    pbuf_free(netPath->generalTxBuf);
    return FALSE;
  }

  /* Open lwIP raw udp interfaces for the general port. */
  netPath->generalPcb = udp_new();
  if(netPath->generalPcb == NULL)
  {
    PERROR("Failed to open General UDP PCB\n");
    udp_remove(netPath->eventPcb);
    pbuf_free(netPath->eventTxBuf);
    pbuf_free(netPath->generalTxBuf);
    return FALSE;
  }

  /* Initialize the buffer queues. */
  netQInit(&netPath->eventQ);
  netQInit(&netPath->generalQ);

  /* Configure network (broadcast/unicast) addresses. */
  netPath->unicastAddr = 0;
  if(!lookupSubdomainAddress(rtOpts->subdomainName, addrStr))
  {
    udp_disconnect(netPath->eventPcb);
    udp_disconnect(netPath->generalPcb);
    udp_remove(netPath->eventPcb);
    udp_remove(netPath->generalPcb);
    pbuf_free(netPath->eventTxBuf);
    pbuf_free(netPath->generalTxBuf);
    return FALSE;
  }
  if(!inet_aton(addrStr, &netAddr))
  {
    ERROR("failed to encode multi-cast address: %s\n", addrStr);
    udp_disconnect(netPath->eventPcb);
    udp_disconnect(netPath->generalPcb);
    udp_remove(netPath->eventPcb);
    udp_remove(netPath->generalPcb);
    pbuf_free(netPath->eventTxBuf);
    pbuf_free(netPath->generalTxBuf);
    return FALSE;
  }
  netPath->multicastAddr = netAddr.s_addr;

  /* Setup subdomain address string. */
  for(i = 0; i < SUBDOMAIN_ADDRESS_LENGTH; ++i)
  {
    ptpClock->subdomain_address[i] = (netAddr.s_addr >> (i * 8)) & 0xff;
  }

  /* Establish the appropriate UDP bindings/connections for events. */

  udp_recv(netPath->eventPcb, eventRecv, netPath);
  udp_bind(netPath->eventPcb, IP_ADDR_ANY, PTP_EVENT_PORT);
//  udp_connect(netPath->eventPcb, IP_ADDR_ANY, PTP_EVENT_PORT);
  *(Integer16*)ptpClock->event_port_address = PTP_EVENT_PORT;

  /* Establish the appropriate UDP bindings/connections for general. */
  udp_recv(netPath->generalPcb, generalRecv, netPath);
  udp_bind(netPath->generalPcb, IP_ADDR_ANY, PTP_GENERAL_PORT);
//  udp_connect(netPath->generalPcb, IP_ADDR_ANY, PTP_GENERAL_PORT);
  *(Integer16*)ptpClock->general_port_address = PTP_GENERAL_PORT;

  /* Return a success code. */
  return TRUE;
}

/* shut down the UDP stuff */
Boolean netShutdown(NetPath *netPath)
{
  /* Disconnect and close the Event UDP interface */
  if(netPath->eventPcb)
  {
    udp_disconnect(netPath->eventPcb);
    udp_remove(netPath->eventPcb);
  }

  /* Disconnect and close the General UDP interface */
  if(netPath->generalPcb)
  {
    udp_disconnect(netPath->generalPcb);
    udp_remove(netPath->generalPcb);
  }

  /* Free up the Event and General Tx PBUFs. */
  if(netPath->eventTxBuf)
  {
    pbuf_free(netPath->eventTxBuf);
  }
  if(netPath->generalTxBuf)
  {
    pbuf_free(netPath->generalTxBuf);
  }

  /* Clear the network addresses. */
  netPath->multicastAddr = 0;
  netPath->unicastAddr = 0;

  /* Return a success code. */
  return TRUE;
}

/* Wait for a packet to come in on either port.  For now, there is no wait.
 * Simply check to see if a packet is available on either port and return 1,
 * otherwise return 0. */
int netSelect(TimeInternal *timeout, NetPath *netPath)
{
  /* Check the packet queues.  If there is data, return TRUE. */
  if(netQCheck(&netPath->eventQ) || netQCheck(&netPath->generalQ))
    return 1;
  return 0;
}

/* Pop a message off of the event queue and copy it to the passed in buffer. */
size_t netRecvEvent(Octet *buf, TimeInternal *time, NetPath *netPath)
{
  int i, j;
  int iPacketSize;
  struct pbuf *p, *pcopy;

  /* Attempt to get a buffer from the Q.  If none is available,
   * return length of 0. */

  p = netQGet(&netPath->eventQ);
  if(p == NULL)
    return 0;
  pcopy = p;


  /* Here, p points to a valid PBUF structure.  Verify that we have
   * enough space to store the contents. */
  if(p->tot_len > PACKET_SIZE)
  {
    ERROR("received truncated message\n");
    return 0;
  }

  /* Copy the PBUF payload into the buffer. */
  j = 0;
  iPacketSize = p->tot_len;
  for(i = 0; i < iPacketSize; i++)
  {
    buf[i] = ((u8_t *)pcopy->payload)[j++];
    if(j == pcopy->len)
    {
      pcopy = pcopy->next;
      j = 0;
    }
  }

  /* Get the timestamp information. */
  time->seconds = p->time_s;
  time->nanoseconds = p->time_ns;

  /* Free up the pbuf (chain). */
  pbuf_free(p);

  /* Return the length of data copied. */
  return iPacketSize;
}

/* Pop a message off of the general queue and copy to the passed in buffer. */
size_t netRecvGeneral(Octet *buf, NetPath *netPath)
{
  int i, j;
  int iPacketSize;
  struct pbuf *p, *pcopy;


  /* Attempt to get a buffer from the Q.  If none is available,
   * return length of 0. */
  p = netQGet(&netPath->generalQ);
  if(p == NULL)
    return 0;
  pcopy = p;

  /* Here, p points to a valid PBUF structure.  Verify that we have
   * enough space to store the contents. */
  if(p->tot_len > PACKET_SIZE)
  {
    ERROR("received truncated message\n");
    return 0;
  }

  /* Copy the PBUF payload into the buffer. */
  j = 0;
  iPacketSize = p->tot_len;
  for(i = 0; i < iPacketSize; i++)
  {
    buf[i] = ((u8_t *)pcopy->payload)[j++];
    if(j == pcopy->len)
    {
      pcopy = pcopy->next;
      j = 0;
    }
  }

  /* Free up the pbuf (chain). */
  pbuf_free(p);

  /* Return the length of data copied. */
  return iPacketSize;
}

/* Transmit a packet on the Event Port. */
size_t netSendEvent(Octet *buf, UInteger16 length, NetPath *netPath)
{
  int i, j;
  struct pbuf *pcopy;

  /* Reallocate the tx pbuf based on the current size. */
  pbuf_realloc(netPath->eventTxBuf, length);
  pcopy = netPath->eventTxBuf;

  /* Copy the incoming data into the pbuf payload. */
  j = 0;
  for(i = 0; i < length; i++)
  {
    ((u8_t *)pcopy->payload)[j++] = buf[i];
    if(j == pcopy->len)
    {
        pcopy = pcopy->next;
        j = 0;
    }
  }

  /* send the buffer. */
  udp_sendto(netPath->eventPcb, netPath->eventTxBuf,
    (void *)&netPath->multicastAddr, PTP_EVENT_PORT);

  return(length);
}

/* Transmit a packet on the General Port. */
size_t netSendGeneral(Octet *buf, UInteger16 length, NetPath *netPath)
{
  int i, j;
  struct pbuf *pcopy;

  /* Reallocate the tx pbuf based on the current size. */
  pbuf_realloc(netPath->generalTxBuf, length);
  pcopy = netPath->generalTxBuf;

  /* Copy the incoming data into the pbuf payload. */
  j = 0;
  for(i = 0; i < length; i++)
  {
    ((u8_t *)pcopy->payload)[j++] = buf[i];
    if(j == pcopy->len)
    {
      pcopy = pcopy->next;
      j = 0;
    }
  }

  /* send the buffer. */
  udp_sendto(netPath->eventPcb, netPath->generalTxBuf,
    (void *)&netPath->multicastAddr, PTP_GENERAL_PORT);

  return(length);
}
