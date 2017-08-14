/* Copyright (c) 2008 Nokia Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PROFILETIMER_H_
#define PROFILETIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PROFILE_LOG
void end_time(int , char *);
#define TEST_PROBE end_time(__LINE__, __FILE__)
#else
#define TEST_PROBE
#endif

#ifdef __cplusplus
}
#endif

#endif  //PROFILETIMER_H_