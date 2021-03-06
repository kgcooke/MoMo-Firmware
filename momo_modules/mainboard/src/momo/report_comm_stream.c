#include "report_comm_stream.h"
#include "momo_config.h"
#include "module_manager.h"
#include "scheduler.h"
#include "bus_master.h"
#include "system_log.h"
#include "utilities.h"
#include <string.h>

#define CONFIG current_momo_state.report_config

#define RETRY_MAX 5

static char* report_buffer;
static uint8 report_stream_offset;
ModuleIterator comm_module_iterator;
bool current_stream_finished;
uint8 retry_count = 0;

ScheduledTask report_retry_task;

void receive_gsm_stream_response(unsigned char a);

void init_comm_stream()
{
  report_retry_task.flags = 0; //Make sure we initialize this to avoid corrupting the scheduler

  comm_module_iterator = create_module_iterator( kMIBCommunicationType );
}

void report_rpc( MIBUnified *cmd, uint8 command, uint8 spec )
{
  cmd->address = module_iter_address( &comm_module_iterator );
  cmd->bus_command.feature = 11;
  cmd->bus_command.command = command;
  cmd->bus_command.param_spec = spec;
  bus_master_rpc_async(receive_gsm_stream_response, cmd); //TODO: Handle failure to send
}
void reset_comm_stream()
{
  report_stream_offset = 0;
  current_stream_finished = false;
}
void open_stream()
{
  write_system_logf( kDebugLog, "Opening comm stream (module: %d, route: %s)", module_iter_address( &comm_module_iterator ), CONFIG.report_server_address );

  MIBUnified cmd;
  memcpy( cmd.mib_buffer, CONFIG.report_server_address, strlen(CONFIG.report_server_address) );
  report_rpc( &cmd, 0, plist_with_buffer(0,strlen(CONFIG.report_server_address)) );
}
void next_comm_module( void* arg )
{
  module_iter_next( &comm_module_iterator );
  reset_comm_stream();
  retry_count = 0;
  if ( module_iter_get( &comm_module_iterator ) != NULL )
  {
    open_stream();
  }
  else
  {
    DEBUG_LOGL( "Finished streaming report to all comm modules." );
  }
}

void stream_to_gsm() {
  MIBUnified cmd;
  if ( report_stream_offset >= strlen(report_buffer) )
  {
    DEBUG_LOGL( "Closing comm stream." );
    current_stream_finished = true;
    report_rpc( &cmd, 2, plist_empty() );
    return;
  }

  DEBUG_LOGL( "Streaming data..." );
  uint8 byte_count = strlen(report_buffer)-report_stream_offset;
  if ( byte_count > kBusMaxMessageSize )
    byte_count = kBusMaxMessageSize;
  memcpy( cmd.mib_buffer, report_buffer+report_stream_offset, byte_count );
  DEBUG_LOG( cmd.mib_buffer, byte_count );
  report_stream_offset += byte_count;
  report_rpc( &cmd, 1, plist_with_buffer( 0, byte_count ) );
}
void receive_gsm_stream_response(unsigned char a) 
{
  FLUSH_LOG();
  if ( a != kNoMIBError )
  {
    write_system_logf( kCriticalLog, "Failed to send a message to a comm module!  Error: %d", a );
    notify_report_failure();
  }
  else if ( !current_stream_finished )
  {
    taskloop_add( stream_to_gsm, NULL );
  }
  // else wait for success/failure notification
  FLUSH_LOG();
}

void report_stream_abandon()
{
  if ( module_iter_get( &comm_module_iterator ) != NULL )
  {
    DEBUG_LOGL( "Abandoning current report stream." );
    current_stream_finished = true;
    
    MIBUnified cmd;
    cmd.address = module_iter_address( &comm_module_iterator );
    cmd.bus_command.feature = 11;
    cmd.bus_command.command = 3;
    cmd.bus_command.param_spec = plist_empty();
    bus_master_rpc_async(NULL, &cmd);

    while ( module_iter_get( &comm_module_iterator ) != NULL )
      module_iter_next( &comm_module_iterator );
  }

  scheduler_remove_task( &report_retry_task );
}
void report_stream_send( char* buffer )
{
  report_buffer = buffer;
  comm_module_iterator = create_module_iterator( kMIBCommunicationType );
  taskloop_add( next_comm_module, NULL ); 
}

void notify_report_success()
{
  // TODO: Save success or failure to the report log.
  DEBUG_LOGL( "Report succeeded." );
  taskloop_add( next_comm_module, NULL );
}

void notify_report_failure()
{
  // TODO: Save success or failure to the report log.
  if ( retry_count < RETRY_MAX )
  {
    int retry_interval = CONFIG.report_interval -1;
    if (retry_interval < 0)
      retry_interval = 0;

    ++retry_count;
    DEBUG_LOGL( "Report failed.  Retrying." );
    reset_comm_stream();
    scheduler_schedule_task( open_stream, retry_interval, 1, &report_retry_task, NULL ); // if we're reporting every day, retry every hour
    // TODO: This blocks streaming to any other module, which could be problematic
  }
  else
  {
    DEBUG_LOGL( "Retry count exceeded, abandoning report." );
    taskloop_add( next_comm_module, NULL );
  }
}