/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2007 Toni Spets
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "json/json.h"
#include <assert.h>
#include <time.h>
#include <cmath>
#include <mutex>
#include "ccurl_thread.h"
#include "conky.h"
#include "logging.h"
#include "text_object.h"
#include <stdlib.h>


namespace{

class bitpanda_curl_cb : public curl_callback<std::string> {
  typedef curl_callback<std::string> Base;

 protected:
  virtual void process_data() {
    std::lock_guard<std::mutex> lock(result_mutex);
    result = data;
  }

 public:
  bitpanda_curl_cb(uint32_t period, const std::string &uri)
      : Base(period, Tuple(uri)) {}
};
}
#define BITPANDA_API_URI "https://api.exchange.bitpanda.com/public/v1/market-ticker"
//#define BITPANDA_API_URI "https://warpenguin.dev/ip"

struct bitpanda_data {
  char currency[64];
  int ownedmicros;
  int interval;
};

static void bitpanda_process_info(char *p, int p_max_size, int interval, int ownedmicros, char* currency) {
  char *str;

  uint32_t period = std::max(lround(interval / active_update_interval()), 1l);

  auto cb = conky::register_cb<bitpanda_curl_cb>(period, BITPANDA_API_URI);


  std::string data = (std::string)cb->get_result();

  if(!data.length()){
    *p = 0;
    return;
  }
  
  const auto rawJsonLength = static_cast<int>(data.length());
  JSONCPP_STRING err;
  Json::Value root;
  Json::CharReaderBuilder builder;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(data.c_str(), data.c_str() + rawJsonLength, &root, &err)) {
      NORM_ERR("Failed to parse json");
      return;
  }
  Json::Value currency_object;
  for( Json::Value::const_iterator it = root.begin(); it != root.end(); it++){
    if(!strncmp((*it)["instrument_code"].asString().c_str(), currency, 7)){
      currency_object = (*it);
      break;
    }
  }
  if(currency_object.type() != Json::objectValue || currency_object.size() == 0){
    NORM_ERR("Currency %s not found in response", currency);
  }

  const std::string last_price_str = currency_object["last_price"].asString();
  double last_price = atof(last_price_str.c_str());
  const std::string time_string = currency_object["time"].asString();
  struct tm tm;
  char time[64];
  memset(&tm, 0, sizeof(tm));
  strptime(time_string.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
  strftime(time, 64, "@%H:%M", &tm);
  const std::string change = currency_object["price_change_percentage"].asString();


  snprintf(p, p_max_size, "%s %0.2f€ %0.2f€ %s%%", time, last_price, last_price*ownedmicros/1000000.0,  change.c_str());
}

void bitpanda_scan_arg(struct text_object *obj, const char *arg) {
  int argc;
  struct bitpanda_data *bd;

  bd = (struct bitpanda_data *)malloc(sizeof(struct bitpanda_data));
  memset(bd, 0, sizeof(struct bitpanda_data));

  argc = sscanf(arg, "%d %63s %d",  &bd->interval, bd->currency, &bd->ownedmicros);
  if (argc < 3) {
    NORM_ERR("wrong number of arguments for $bitpanda");
    free(bd);
    return;
  }
  obj->data.opaque = bd;
}

void bitpanda_print_info(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct bitpanda_data *bd = (struct bitpanda_data *)obj->data.opaque;

  if (!bd) {
    NORM_ERR("error processing BitPanda data");
    return;
  }
  bitpanda_process_info(p, p_max_size, bd->interval, bd->ownedmicros, bd->currency);
}

void bitpanda_free_obj_info(struct text_object *obj) {
  free_and_zero(obj->data.opaque);
}
