diff -u -r -N ./JAWS3/jaws3/Timer.cpp /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/Timer.cpp
--- ./JAWS3/jaws3/Timer.cpp	Tue Apr 25 16:24:21 2000
+++ /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/Timer.cpp	Wed May 03 10:13:27 2000
@@ -1,11 +1,11 @@
-// $Id: jaws3-msvc.pat 80826 2008-03-04 14:51:23Z wotte $
+// $Id: jaws3-msvc.pat 80826 2008-03-04 14:51:23Z wotte $
 
 #define JAWS_BUILD_DLL
 
 #include "jaws3/Timer.h"
 #include "jaws3/Task_Timer.h"
 
-JAWS_Timer::JAWS_Timer (JAWS_Timer_Impl *impl = 0)
+JAWS_Timer::JAWS_Timer (JAWS_Timer_Impl *impl )
   : impl_ (impl)
 {
   // TODO: Change this to use JAWS_Options after we have more than
diff -u -r -N ./JAWS3/jaws3/jaws3.dep /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.dep
--- ./JAWS3/jaws3/jaws3.dep	Wed Dec 31 19:00:00 1969
+++ /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.dep	Wed May 03 10:12:04 2000
@@ -0,0 +1,2193 @@
+# Microsoft Developer Studio Generated Dependency File, included by jaws3.mak
+
+.\Asynch_IO.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\asynch_io.h"\
+	"..\..\..\ace\Asynch_IO.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Asynch_IO.h"\
+	".\Asynch_IO_Helpers.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\IO.h"\
+	".\Reactive_IO.h"\
+	
+
+.\Concurrency.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Concurrency.h"\
+	".\Config_File.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\Options.h"\
+	".\Protocol_Handler.h"\
+	".\TPOOL_Concurrency.h"\
+	".\TPR_Concurrency.h"\
+	
+
+.\Config_File.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\addr.h"\
+	"..\..\..\ace\Addr.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\file.h"\
+	"..\..\..\ace\file.i"\
+	"..\..\..\ace\file_addr.h"\
+	"..\..\..\ace\FILE_Addr.i"\
+	"..\..\..\ace\file_connector.h"\
+	"..\..\..\ace\file_connector.i"\
+	"..\..\..\ace\file_io.h"\
+	"..\..\..\ace\file_io.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_sap.h"\
+	"..\..\..\ace\io_sap.i"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Config_File.h"\
+	".\Export.h"\
+	".\Symbol_Table.h"\
+	
+
+.\Event_Completer.cpp : \
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	
+
+.\Event_Dispatcher.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\asynch_io.h"\
+	"..\..\..\ace\Asynch_IO.i"\
+	"..\..\..\ace\asynch_io_impl.h"\
+	"..\..\..\ace\Asynch_IO_Impl.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\high_res_timer.h"\
+	"..\..\..\ace\High_Res_Timer.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\Local_Tokens.h"\
+	"..\..\..\ace\Local_Tokens.i"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\Map_Manager.cpp"\
+	"..\..\..\ace\Map_Manager.h"\
+	"..\..\..\ace\Map_Manager.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\pipe.h"\
+	"..\..\..\ace\pipe.i"\
+	"..\..\..\ace\proactor.h"\
+	"..\..\..\ace\Proactor.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\select_reactor.h"\
+	"..\..\..\ace\Select_Reactor.i"\
+	"..\..\..\ace\select_reactor_base.h"\
+	"..\..\..\ace\Select_Reactor_Base.i"\
+	"..\..\..\ace\select_reactor_t.cpp"\
+	"..\..\..\ace\select_reactor_t.h"\
+	"..\..\..\ace\select_reactor_t.i"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_heap.h"\
+	"..\..\..\ace\timer_heap_t.cpp"\
+	"..\..\..\ace\timer_heap_t.h"\
+	"..\..\..\ace\timer_list.h"\
+	"..\..\..\ace\timer_list_t.cpp"\
+	"..\..\..\ace\timer_list_t.h"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\timer_wheel.h"\
+	"..\..\..\ace\timer_wheel_t.cpp"\
+	"..\..\..\ace\timer_wheel_t.h"\
+	"..\..\..\ace\token.h"\
+	"..\..\..\ace\Token.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Dispatcher.h"\
+	".\Export.h"\
+	
+
+.\FILE.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\addr.h"\
+	"..\..\..\ace\Addr.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\file.h"\
+	"..\..\..\ace\file.i"\
+	"..\..\..\ace\file_addr.h"\
+	"..\..\..\ace\FILE_Addr.i"\
+	"..\..\..\ace\file_io.h"\
+	"..\..\..\ace\file_io.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_sap.h"\
+	"..\..\..\ace\io_sap.i"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Export.h"\
+	".\FILE.h"\
+	
+
+.\IO.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\asynch_io.h"\
+	"..\..\..\ace\Asynch_IO.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Asynch_IO.h"\
+	".\Config_File.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\IO.h"\
+	".\Options.h"\
+	".\Reactive_IO.h"\
+	".\Synch_IO.h"\
+	
+
+.\main.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\Local_Tokens.h"\
+	"..\..\..\ace\Local_Tokens.i"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\Map_Manager.cpp"\
+	"..\..\..\ace\Map_Manager.h"\
+	"..\..\..\ace\Map_Manager.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\pipe.h"\
+	"..\..\..\ace\pipe.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\select_reactor.h"\
+	"..\..\..\ace\Select_Reactor.i"\
+	"..\..\..\ace\select_reactor_base.h"\
+	"..\..\..\ace\Select_Reactor_Base.i"\
+	"..\..\..\ace\select_reactor_t.cpp"\
+	"..\..\..\ace\select_reactor_t.h"\
+	"..\..\..\ace\select_reactor_t.i"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\timer_heap.h"\
+	"..\..\..\ace\timer_heap_t.cpp"\
+	"..\..\..\ace\timer_heap_t.h"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\token.h"\
+	"..\..\..\ace\Token.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Dispatcher.h"\
+	".\Export.h"\
+	
+
+.\Options.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Config_File.h"\
+	".\Export.h"\
+	".\Options.h"\
+	
+
+.\Protocol_Handler.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Concurrency.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\Protocol_Handler.h"\
+	
+
+.\Reactive_IO.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\IO.h"\
+	".\Reactive_IO.h"\
+	".\Reactive_IO_Helpers.h"\
+	
+
+.\Symbol_Table.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Export.h"\
+	".\Symbol_Table.h"\
+	
+
+.\Synch_IO.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\IO.h"\
+	".\Synch_IO.h"\
+	
+
+.\Task_Timer.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\high_res_timer.h"\
+	"..\..\..\ace\High_Res_Timer.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_adapters.cpp"\
+	"..\..\..\ace\timer_queue_adapters.h"\
+	"..\..\..\ace\timer_queue_adapters.i"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\timer_wheel.h"\
+	"..\..\..\ace\timer_wheel_t.cpp"\
+	"..\..\..\ace\timer_wheel_t.h"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\Task_Timer.h"\
+	".\Timer.h"\
+	".\Timer_Helpers.h"\
+	
+
+.\Templates.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\addr.h"\
+	"..\..\..\ace\Addr.i"\
+	"..\..\..\ace\asynch_io.h"\
+	"..\..\..\ace\Asynch_IO.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\dynamic.h"\
+	"..\..\..\ace\Dynamic.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\high_res_timer.h"\
+	"..\..\..\ace\High_Res_Timer.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\inet_addr.h"\
+	"..\..\..\ace\INET_Addr.i"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\ipc_sap.h"\
+	"..\..\..\ace\ipc_sap.i"\
+	"..\..\..\ace\Local_Tokens.h"\
+	"..\..\..\ace\Local_Tokens.i"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\lsock.h"\
+	"..\..\..\ace\LSOCK.i"\
+	"..\..\..\ace\lsock_stream.h"\
+	"..\..\..\ace\LSOCK_Stream.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\Map_Manager.cpp"\
+	"..\..\..\ace\Map_Manager.h"\
+	"..\..\..\ace\Map_Manager.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\pipe.h"\
+	"..\..\..\ace\pipe.i"\
+	"..\..\..\ace\qos_session.h"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\select_reactor.h"\
+	"..\..\..\ace\Select_Reactor.i"\
+	"..\..\..\ace\select_reactor_base.h"\
+	"..\..\..\ace\Select_Reactor_Base.i"\
+	"..\..\..\ace\select_reactor_t.cpp"\
+	"..\..\..\ace\select_reactor_t.h"\
+	"..\..\..\ace\select_reactor_t.i"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sock.h"\
+	"..\..\..\ace\sock.i"\
+	"..\..\..\ace\sock_io.h"\
+	"..\..\..\ace\sock_io.i"\
+	"..\..\..\ace\sock_stream.h"\
+	"..\..\..\ace\sock_stream.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\svc_handler.cpp"\
+	"..\..\..\ace\svc_handler.h"\
+	"..\..\..\ace\svc_handler.i"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_heap.h"\
+	"..\..\..\ace\timer_heap_t.cpp"\
+	"..\..\..\ace\timer_heap_t.h"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_adapters.cpp"\
+	"..\..\..\ace\timer_queue_adapters.h"\
+	"..\..\..\ace\timer_queue_adapters.i"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\timer_wheel.h"\
+	"..\..\..\ace\timer_wheel_t.cpp"\
+	"..\..\..\ace\timer_wheel_t.h"\
+	"..\..\..\ace\token.h"\
+	"..\..\..\ace\Token.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\unix_addr.h"\
+	"..\..\..\ace\UNIX_Addr.i"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Asynch_IO.h"\
+	".\Concurrency.h"\
+	".\Config_File.h"\
+	".\Event_Completer.h"\
+	".\Event_Dispatcher.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\IO.h"\
+	".\Options.h"\
+	".\Protocol_Handler.h"\
+	".\Reactive_IO.h"\
+	".\Symbol_Table.h"\
+	".\Synch_IO.h"\
+	".\Task_Timer.h"\
+	".\Timer.h"\
+	".\TPOOL_Concurrency.h"\
+	".\TPR_Concurrency.h"\
+	
+
+.\Timer.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\high_res_timer.h"\
+	"..\..\..\ace\High_Res_Timer.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_adapters.cpp"\
+	"..\..\..\ace\timer_queue_adapters.h"\
+	"..\..\..\ace\timer_queue_adapters.i"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\timer_wheel.h"\
+	"..\..\..\ace\timer_wheel_t.cpp"\
+	"..\..\..\ace\timer_wheel_t.h"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\Task_Timer.h"\
+	".\Timer.h"\
+	
+
+.\Timer_Helpers.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\high_res_timer.h"\
+	"..\..\..\ace\High_Res_Timer.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_adapters.cpp"\
+	"..\..\..\ace\timer_queue_adapters.h"\
+	"..\..\..\ace\timer_queue_adapters.i"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\timer_wheel.h"\
+	"..\..\..\ace\timer_wheel_t.cpp"\
+	"..\..\..\ace\timer_wheel_t.h"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\Timer.h"\
+	".\Timer_Helpers.h"\
+	
+
+.\TPOOL_Concurrency.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Concurrency.h"\
+	".\Config_File.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\Options.h"\
+	".\Protocol_Handler.h"\
+	".\TPOOL_Concurrency.h"\
+	
+
+.\TPR_Concurrency.cpp : \
+	"..\..\..\ace\ace.h"\
+	"..\..\..\ace\ace.i"\
+	"..\..\..\ace\atomic_op.i"\
+	"..\..\..\ace\based_pointer_repository.h"\
+	"..\..\..\ace\based_pointer_t.cpp"\
+	"..\..\..\ace\based_pointer_t.h"\
+	"..\..\..\ace\based_pointer_t.i"\
+	"..\..\..\ace\basic_types.h"\
+	"..\..\..\ace\Basic_Types.i"\
+	"..\..\..\ace\config-win32-borland.h"\
+	"..\..\..\ace\config-win32-common.h"\
+	"..\..\..\ace\config-win32-msvc.h"\
+	"..\..\..\ace\config-win32-visualage.h"\
+	"..\..\..\ace\config-win32.h"\
+	"..\..\..\ace\config-WinCE.h"\
+	"..\..\..\ace\config.h"\
+	"..\..\..\ace\containers.h"\
+	"..\..\..\ace\Containers.i"\
+	"..\..\..\ace\containers_t.cpp"\
+	"..\..\..\ace\containers_t.h"\
+	"..\..\..\ace\containers_t.i"\
+	"..\..\..\ace\event_handler.h"\
+	"..\..\..\ace\Event_Handler.i"\
+	"..\..\..\ace\free_list.cpp"\
+	"..\..\..\ace\free_list.h"\
+	"..\..\..\ace\free_list.i"\
+	"..\..\..\ace\functor.h"\
+	"..\..\..\ace\Functor.i"\
+	"..\..\..\ace\functor_t.cpp"\
+	"..\..\..\ace\functor_t.h"\
+	"..\..\..\ace\functor_t.i"\
+	"..\..\..\ace\handle_set.h"\
+	"..\..\..\ace\Handle_Set.i"\
+	"..\..\..\ace\hash_map_manager.h"\
+	"..\..\..\ace\Hash_Map_Manager.i"\
+	"..\..\..\ace\hash_map_manager_t.cpp"\
+	"..\..\..\ace\hash_map_manager_t.h"\
+	"..\..\..\ace\hash_map_manager_t.i"\
+	"..\..\..\ace\inc_user_config.h"\
+	"..\..\..\ace\io_cntl_msg.h"\
+	"..\..\..\ace\iosfwd.h"\
+	"..\..\..\ace\log_msg.h"\
+	"..\..\..\ace\log_priority.h"\
+	"..\..\..\ace\log_record.h"\
+	"..\..\..\ace\log_record.i"\
+	"..\..\..\ace\Malloc.h"\
+	"..\..\..\ace\Malloc.i"\
+	"..\..\..\ace\malloc_base.h"\
+	"..\..\..\ace\malloc_t.cpp"\
+	"..\..\..\ace\malloc_t.h"\
+	"..\..\..\ace\malloc_t.i"\
+	"..\..\..\ace\managed_object.cpp"\
+	"..\..\..\ace\managed_object.h"\
+	"..\..\..\ace\managed_object.i"\
+	"..\..\..\ace\mem_map.h"\
+	"..\..\..\ace\Mem_Map.i"\
+	"..\..\..\ace\memory_pool.h"\
+	"..\..\..\ace\Memory_Pool.i"\
+	"..\..\..\ace\message_block.h"\
+	"..\..\..\ace\Message_Block.i"\
+	"..\..\..\ace\message_block_t.cpp"\
+	"..\..\..\ace\message_block_t.h"\
+	"..\..\..\ace\message_block_t.i"\
+	"..\..\..\ace\message_queue.h"\
+	"..\..\..\ace\Message_Queue.i"\
+	"..\..\..\ace\message_queue_t.cpp"\
+	"..\..\..\ace\message_queue_t.h"\
+	"..\..\..\ace\message_queue_t.i"\
+	"..\..\..\ace\min_max.h"\
+	"..\..\..\ace\module.cpp"\
+	"..\..\..\ace\module.h"\
+	"..\..\..\ace\module.i"\
+	"..\..\..\ace\object_manager.h"\
+	"..\..\..\ace\Object_Manager.i"\
+	"..\..\..\ace\os.h"\
+	"..\..\..\ace\OS.i"\
+	"..\..\..\ace\reactor.h"\
+	"..\..\..\ace\Reactor.i"\
+	"..\..\..\ace\reactor_impl.h"\
+	"..\..\..\ace\service_config.h"\
+	"..\..\..\ace\Service_Config.i"\
+	"..\..\..\ace\service_object.h"\
+	"..\..\..\ace\Service_Object.i"\
+	"..\..\..\ace\service_repository.h"\
+	"..\..\..\ace\Service_Repository.i"\
+	"..\..\..\ace\service_types.h"\
+	"..\..\..\ace\Service_Types.i"\
+	"..\..\..\ace\shared_object.h"\
+	"..\..\..\ace\Shared_Object.i"\
+	"..\..\..\ace\Signal.h"\
+	"..\..\..\ace\Signal.i"\
+	"..\..\..\ace\singleton.cpp"\
+	"..\..\..\ace\singleton.h"\
+	"..\..\..\ace\singleton.i"\
+	"..\..\..\ace\sstring.h"\
+	"..\..\..\ace\SString.i"\
+	"..\..\..\ace\strategies.h"\
+	"..\..\..\ace\Strategies.i"\
+	"..\..\..\ace\strategies_t.cpp"\
+	"..\..\..\ace\strategies_t.h"\
+	"..\..\..\ace\strategies_t.i"\
+	"..\..\..\ace\stream_modules.cpp"\
+	"..\..\..\ace\stream_modules.h"\
+	"..\..\..\ace\stream_modules.i"\
+	"..\..\..\ace\streams.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.h"\
+	"..\..\..\ace\SV_Semaphore_Complex.i"\
+	"..\..\..\ace\SV_Semaphore_Simple.h"\
+	"..\..\..\ace\SV_Semaphore_Simple.i"\
+	"..\..\..\ace\svc_conf_tokens.h"\
+	"..\..\..\ace\synch.h"\
+	"..\..\..\ace\Synch.i"\
+	"..\..\..\ace\synch_options.h"\
+	"..\..\..\ace\Synch_Options.i"\
+	"..\..\..\ace\synch_t.cpp"\
+	"..\..\..\ace\synch_t.h"\
+	"..\..\..\ace\synch_t.i"\
+	"..\..\..\ace\task.h"\
+	"..\..\..\ace\Task.i"\
+	"..\..\..\ace\task_t.cpp"\
+	"..\..\..\ace\task_t.h"\
+	"..\..\..\ace\task_t.i"\
+	"..\..\..\ace\thread.h"\
+	"..\..\..\ace\Thread.i"\
+	"..\..\..\ace\thread_manager.h"\
+	"..\..\..\ace\Thread_Manager.i"\
+	"..\..\..\ace\timer_queue.h"\
+	"..\..\..\ace\timer_queue_t.cpp"\
+	"..\..\..\ace\timer_queue_t.h"\
+	"..\..\..\ace\timer_queue_t.i"\
+	"..\..\..\ace\Trace.h"\
+	"..\..\..\ace\wfmo_reactor.h"\
+	"..\..\..\ace\WFMO_Reactor.i"\
+	"..\..\..\ace\ws2tcpip.h"\
+	".\Concurrency.h"\
+	".\Config_File.h"\
+	".\Event_Completer.h"\
+	".\Event_Result.h"\
+	".\Export.h"\
+	".\Options.h"\
+	".\Protocol_Handler.h"\
+	".\TPR_Concurrency.h"\
+	
diff -u -r -N ./JAWS3/jaws3/jaws3.dsp /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.dsp
--- ./JAWS3/jaws3/jaws3.dsp	Wed Dec 31 19:00:00 1969
+++ /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.dsp	Wed May 03 14:22:09 2000
@@ -0,0 +1,258 @@
+# Microsoft Developer Studio Project File - Name="jaws3" - Package Owner=<4>
+# Microsoft Developer Studio Generated Build File, Format Version 6.00
+# ** DO NOT EDIT **
+
+# TARGTYPE "Win32 (x86) Console Application" 0x0103
+
+CFG=jaws3 - Win32 Debug
+!MESSAGE This is not a valid makefile. To build this project using NMAKE,
+!MESSAGE use the Export Makefile command and run
+!MESSAGE 
+!MESSAGE NMAKE /f "jaws3.mak".
+!MESSAGE 
+!MESSAGE You can specify a configuration when running NMAKE
+!MESSAGE by defining the macro CFG on the command line. For example:
+!MESSAGE 
+!MESSAGE NMAKE /f "jaws3.mak" CFG="jaws3 - Win32 Debug"
+!MESSAGE 
+!MESSAGE Possible choices for configuration are:
+!MESSAGE 
+!MESSAGE "jaws3 - Win32 Release" (based on "Win32 (x86) Console Application")
+!MESSAGE "jaws3 - Win32 Debug" (based on "Win32 (x86) Console Application")
+!MESSAGE 
+
+# Begin Project
+# PROP AllowPerConfigDependencies 0
+# PROP Scc_ProjName ""
+# PROP Scc_LocalPath ""
+CPP=cl.exe
+RSC=rc.exe
+
+!IF  "$(CFG)" == "jaws3 - Win32 Release"
+
+# PROP BASE Use_MFC 0
+# PROP BASE Use_Debug_Libraries 0
+# PROP BASE Output_Dir "Release"
+# PROP BASE Intermediate_Dir "Release"
+# PROP BASE Target_Dir ""
+# PROP Use_MFC 0
+# PROP Use_Debug_Libraries 0
+# PROP Output_Dir ""
+# PROP Intermediate_Dir "Release"
+# PROP Ignore_Export_Lib 0
+# PROP Target_Dir ""
+# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
+# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\.." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
+# ADD BASE RSC /l 0x409 /d "NDEBUG"
+# ADD RSC /l 0x409 /d "NDEBUG"
+BSC32=bscmake.exe
+# ADD BASE BSC32 /nologo
+# ADD BSC32 /nologo
+LINK32=link.exe
+# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
+# ADD LINK32 ace.lib /nologo /subsystem:console /machine:I386 /out:"jaws3-r.exe" /libpath:"..\..\..\ace"
+
+!ELSEIF  "$(CFG)" == "jaws3 - Win32 Debug"
+
+# PROP BASE Use_MFC 0
+# PROP BASE Use_Debug_Libraries 1
+# PROP BASE Output_Dir "Debug"
+# PROP BASE Intermediate_Dir "Debug"
+# PROP BASE Target_Dir ""
+# PROP Use_MFC 0
+# PROP Use_Debug_Libraries 1
+# PROP Output_Dir ""
+# PROP Intermediate_Dir "Debug"
+# PROP Ignore_Export_Lib 0
+# PROP Target_Dir ""
+# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
+# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\.." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
+# ADD BASE RSC /l 0x409 /d "_DEBUG"
+# ADD RSC /l 0x409 /d "_DEBUG"
+BSC32=bscmake.exe
+# ADD BASE BSC32 /nologo
+# ADD BSC32 /nologo
+LINK32=link.exe
+# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
+# ADD LINK32 aced.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\ace"
+
+!ENDIF 
+
+# Begin Target
+
+# Name "jaws3 - Win32 Release"
+# Name "jaws3 - Win32 Debug"
+# Begin Group "Source Files"
+
+# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
+# Begin Source File
+
+SOURCE=.\Asynch_IO.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Concurrency.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Config_File.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Event_Completer.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Event_Dispatcher.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\FILE.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\IO.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\main.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Options.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Protocol_Handler.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Reactive_IO.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Symbol_Table.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Synch_IO.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Task_Timer.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Templates.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Timer.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\Timer_Helpers.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\TPOOL_Concurrency.cpp
+# End Source File
+# Begin Source File
+
+SOURCE=.\TPR_Concurrency.cpp
+# End Source File
+# End Group
+# Begin Group "Header Files"
+
+# PROP Default_Filter "h;hpp;hxx;hm;inl"
+# Begin Source File
+
+SOURCE=.\Asynch_IO.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Asynch_IO_Helpers.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Concurrency.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Config_File.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Event_Completer.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Event_Dispatcher.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Event_Result.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Export.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\FILE.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\IO.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Options.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Protocol_Handler.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Reactive_IO.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Reactive_IO_Helpers.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Symbol_Table.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Synch_IO.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Task_Timer.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Timer.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\Timer_Helpers.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\TPOOL_Concurrency.h
+# End Source File
+# Begin Source File
+
+SOURCE=.\TPR_Concurrency.h
+# End Source File
+# End Group
+# Begin Group "Resource Files"
+
+# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
+# End Group
+# End Target
+# End Project
diff -u -r -N ./JAWS3/jaws3/jaws3.dsw /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.dsw
--- ./JAWS3/jaws3/jaws3.dsw	Wed Dec 31 19:00:00 1969
+++ /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.dsw	Wed May 03 10:12:15 2000
@@ -0,0 +1,29 @@
+Microsoft Developer Studio Workspace File, Format Version 6.00
+# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!
+
+###############################################################################
+
+Project: "jaws3"=.\jaws3.dsp - Package Owner=<4>
+
+Package=<5>
+{{{
+}}}
+
+Package=<4>
+{{{
+}}}
+
+###############################################################################
+
+Global:
+
+Package=<5>
+{{{
+}}}
+
+Package=<3>
+{{{
+}}}
+
+###############################################################################
+
diff -u -r -N ./JAWS3/jaws3/jaws3.mak /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.mak
--- ./JAWS3/jaws3/jaws3.mak	Wed Dec 31 19:00:00 1969
+++ /c/dev/Win32/ACE_wrappers/apps/JAWS3/jaws3/jaws3.mak	Wed May 03 14:30:15 2000
@@ -0,0 +1,314 @@
+# Microsoft Developer Studio Generated NMAKE File, Based on jaws3.dsp
+!IF "$(CFG)" == ""
+CFG=jaws3 - Win32 Debug
+!MESSAGE No configuration specified. Defaulting to jaws3 - Win32 Debug.
+!ENDIF 
+
+!IF "$(CFG)" != "jaws3 - Win32 Release" && "$(CFG)" != "jaws3 - Win32 Debug"
+!MESSAGE Invalid configuration "$(CFG)" specified.
+!MESSAGE You can specify a configuration when running NMAKE
+!MESSAGE by defining the macro CFG on the command line. For example:
+!MESSAGE 
+!MESSAGE NMAKE /f "jaws3.mak" CFG="jaws3 - Win32 Debug"
+!MESSAGE 
+!MESSAGE Possible choices for configuration are:
+!MESSAGE 
+!MESSAGE "jaws3 - Win32 Release" (based on "Win32 (x86) Console Application")
+!MESSAGE "jaws3 - Win32 Debug" (based on "Win32 (x86) Console Application")
+!MESSAGE 
+!ERROR An invalid configuration is specified.
+!ENDIF 
+
+!IF "$(OS)" == "Windows_NT"
+NULL=
+!ELSE 
+NULL=nul
+!ENDIF 
+
+CPP=cl.exe
+RSC=rc.exe
+
+!IF  "$(CFG)" == "jaws3 - Win32 Release"
+
+OUTDIR=.
+INTDIR=.\Release
+# Begin Custom Macros
+OutDir=.
+# End Custom Macros
+
+ALL : "$(OUTDIR)\jaws3-r.exe"
+
+
+CLEAN :
+	-@erase "$(INTDIR)\Asynch_IO.obj"
+	-@erase "$(INTDIR)\Concurrency.obj"
+	-@erase "$(INTDIR)\Config_File.obj"
+	-@erase "$(INTDIR)\Event_Completer.obj"
+	-@erase "$(INTDIR)\Event_Dispatcher.obj"
+	-@erase "$(INTDIR)\FILE.obj"
+	-@erase "$(INTDIR)\IO.obj"
+	-@erase "$(INTDIR)\main.obj"
+	-@erase "$(INTDIR)\Options.obj"
+	-@erase "$(INTDIR)\Protocol_Handler.obj"
+	-@erase "$(INTDIR)\Reactive_IO.obj"
+	-@erase "$(INTDIR)\Symbol_Table.obj"
+	-@erase "$(INTDIR)\Synch_IO.obj"
+	-@erase "$(INTDIR)\Task_Timer.obj"
+	-@erase "$(INTDIR)\Templates.obj"
+	-@erase "$(INTDIR)\Timer.obj"
+	-@erase "$(INTDIR)\Timer_Helpers.obj"
+	-@erase "$(INTDIR)\TPOOL_Concurrency.obj"
+	-@erase "$(INTDIR)\TPR_Concurrency.obj"
+	-@erase "$(INTDIR)\vc60.idb"
+	-@erase "$(OUTDIR)\jaws3-r.exe"
+
+"$(INTDIR)" :
+    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"
+
+CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\.." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\jaws3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
+BSC32=bscmake.exe
+BSC32_FLAGS=/nologo /o"$(OUTDIR)\jaws3.bsc" 
+BSC32_SBRS= \
+	
+LINK32=link.exe
+LINK32_FLAGS=ace.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\jaws3-r.pdb" /machine:I386 /out:"$(OUTDIR)\jaws3-r.exe" /libpath:"..\..\..\ace" 
+LINK32_OBJS= \
+	"$(INTDIR)\Asynch_IO.obj" \
+	"$(INTDIR)\Concurrency.obj" \
+	"$(INTDIR)\Config_File.obj" \
+	"$(INTDIR)\Event_Completer.obj" \
+	"$(INTDIR)\Event_Dispatcher.obj" \
+	"$(INTDIR)\FILE.obj" \
+	"$(INTDIR)\IO.obj" \
+	"$(INTDIR)\main.obj" \
+	"$(INTDIR)\Options.obj" \
+	"$(INTDIR)\Protocol_Handler.obj" \
+	"$(INTDIR)\Reactive_IO.obj" \
+	"$(INTDIR)\Symbol_Table.obj" \
+	"$(INTDIR)\Synch_IO.obj" \
+	"$(INTDIR)\Task_Timer.obj" \
+	"$(INTDIR)\Templates.obj" \
+	"$(INTDIR)\Timer.obj" \
+	"$(INTDIR)\Timer_Helpers.obj" \
+	"$(INTDIR)\TPOOL_Concurrency.obj" \
+	"$(INTDIR)\TPR_Concurrency.obj"
+
+"$(OUTDIR)\jaws3-r.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
+    $(LINK32) @<<
+  $(LINK32_FLAGS) $(LINK32_OBJS)
+<<
+
+!ELSEIF  "$(CFG)" == "jaws3 - Win32 Debug"
+
+OUTDIR=.
+INTDIR=.\Debug
+# Begin Custom Macros
+OutDir=.
+# End Custom Macros
+
+ALL : "$(OUTDIR)\jaws3.exe"
+
+
+CLEAN :
+	-@erase "$(INTDIR)\Asynch_IO.obj"
+	-@erase "$(INTDIR)\Concurrency.obj"
+	-@erase "$(INTDIR)\Config_File.obj"
+	-@erase "$(INTDIR)\Event_Completer.obj"
+	-@erase "$(INTDIR)\Event_Dispatcher.obj"
+	-@erase "$(INTDIR)\FILE.obj"
+	-@erase "$(INTDIR)\IO.obj"
+	-@erase "$(INTDIR)\main.obj"
+	-@erase "$(INTDIR)\Options.obj"
+	-@erase "$(INTDIR)\Protocol_Handler.obj"
+	-@erase "$(INTDIR)\Reactive_IO.obj"
+	-@erase "$(INTDIR)\Symbol_Table.obj"
+	-@erase "$(INTDIR)\Synch_IO.obj"
+	-@erase "$(INTDIR)\Task_Timer.obj"
+	-@erase "$(INTDIR)\Templates.obj"
+	-@erase "$(INTDIR)\Timer.obj"
+	-@erase "$(INTDIR)\Timer_Helpers.obj"
+	-@erase "$(INTDIR)\TPOOL_Concurrency.obj"
+	-@erase "$(INTDIR)\TPR_Concurrency.obj"
+	-@erase "$(INTDIR)\vc60.idb"
+	-@erase "$(INTDIR)\vc60.pdb"
+	-@erase "$(OUTDIR)\jaws3.exe"
+	-@erase "$(OUTDIR)\jaws3.ilk"
+	-@erase "$(OUTDIR)\jaws3.pdb"
+
+"$(INTDIR)" :
+    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"
+
+CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\.." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\jaws3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
+BSC32=bscmake.exe
+BSC32_FLAGS=/nologo /o"$(OUTDIR)\jaws3.bsc" 
+BSC32_SBRS= \
+	
+LINK32=link.exe
+LINK32_FLAGS=aced.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\jaws3.pdb" /debug /machine:I386 /out:"$(OUTDIR)\jaws3.exe" /pdbtype:sept /libpath:"..\..\..\ace" 
+LINK32_OBJS= \
+	"$(INTDIR)\Asynch_IO.obj" \
+	"$(INTDIR)\Concurrency.obj" \
+	"$(INTDIR)\Config_File.obj" \
+	"$(INTDIR)\Event_Completer.obj" \
+	"$(INTDIR)\Event_Dispatcher.obj" \
+	"$(INTDIR)\FILE.obj" \
+	"$(INTDIR)\IO.obj" \
+	"$(INTDIR)\main.obj" \
+	"$(INTDIR)\Options.obj" \
+	"$(INTDIR)\Protocol_Handler.obj" \
+	"$(INTDIR)\Reactive_IO.obj" \
+	"$(INTDIR)\Symbol_Table.obj" \
+	"$(INTDIR)\Synch_IO.obj" \
+	"$(INTDIR)\Task_Timer.obj" \
+	"$(INTDIR)\Templates.obj" \
+	"$(INTDIR)\Timer.obj" \
+	"$(INTDIR)\Timer_Helpers.obj" \
+	"$(INTDIR)\TPOOL_Concurrency.obj" \
+	"$(INTDIR)\TPR_Concurrency.obj"
+
+"$(OUTDIR)\jaws3.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
+    $(LINK32) @<<
+  $(LINK32_FLAGS) $(LINK32_OBJS)
+<<
+
+!ENDIF 
+
+.c{$(INTDIR)}.obj::
+   $(CPP) @<<
+   $(CPP_PROJ) $< 
+<<
+
+.cpp{$(INTDIR)}.obj::
+   $(CPP) @<<
+   $(CPP_PROJ) $< 
+<<
+
+.cxx{$(INTDIR)}.obj::
+   $(CPP) @<<
+   $(CPP_PROJ) $< 
+<<
+
+.c{$(INTDIR)}.sbr::
+   $(CPP) @<<
+   $(CPP_PROJ) $< 
+<<
+
+.cpp{$(INTDIR)}.sbr::
+   $(CPP) @<<
+   $(CPP_PROJ) $< 
+<<
+
+.cxx{$(INTDIR)}.sbr::
+   $(CPP) @<<
+   $(CPP_PROJ) $< 
+<<
+
+
+!IF "$(NO_EXTERNAL_DEPS)" != "1"
+!IF EXISTS("jaws3.dep")
+!INCLUDE "jaws3.dep"
+!ELSE 
+!MESSAGE Warning: cannot find "jaws3.dep"
+!ENDIF 
+!ENDIF 
+
+
+!IF "$(CFG)" == "jaws3 - Win32 Release" || "$(CFG)" == "jaws3 - Win32 Debug"
+SOURCE=.\Asynch_IO.cpp
+
+"$(INTDIR)\Asynch_IO.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Concurrency.cpp
+
+"$(INTDIR)\Concurrency.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Config_File.cpp
+
+"$(INTDIR)\Config_File.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Event_Completer.cpp
+
+"$(INTDIR)\Event_Completer.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Event_Dispatcher.cpp
+
+"$(INTDIR)\Event_Dispatcher.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\FILE.cpp
+
+"$(INTDIR)\FILE.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\IO.cpp
+
+"$(INTDIR)\IO.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\main.cpp
+
+"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Options.cpp
+
+"$(INTDIR)\Options.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Protocol_Handler.cpp
+
+"$(INTDIR)\Protocol_Handler.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Reactive_IO.cpp
+
+"$(INTDIR)\Reactive_IO.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Symbol_Table.cpp
+
+"$(INTDIR)\Symbol_Table.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Synch_IO.cpp
+
+"$(INTDIR)\Synch_IO.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Task_Timer.cpp
+
+"$(INTDIR)\Task_Timer.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Templates.cpp
+
+"$(INTDIR)\Templates.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Timer.cpp
+
+"$(INTDIR)\Timer.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\Timer_Helpers.cpp
+
+"$(INTDIR)\Timer_Helpers.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\TPOOL_Concurrency.cpp
+
+"$(INTDIR)\TPOOL_Concurrency.obj" : $(SOURCE) "$(INTDIR)"
+
+
+SOURCE=.\TPR_Concurrency.cpp
+
+"$(INTDIR)\TPR_Concurrency.obj" : $(SOURCE) "$(INTDIR)"
+
+
+
+!ENDIF 
+
