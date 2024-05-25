if (!require(influxdbclient)) install.packages("influxdbclient")
if (!require(fpp3)) install.packages("fpp3")
if (!require(dygraphs)) install.packages("dygraphs")
library(purrr)
library(glue)
library(stringr)
options(browser = "/usr/bin/chromium")

## You can generate an API token from the "API Tokens Tab" in the UI
## In your R console you can set token as

token <- Sys.getenv("INFLUX_TOKEN")

client <- InfluxDBClient$new(
  url = "http://iot.unloquer.org:8086",
  token = token,
  org = "unloquer"
)

variables_names <- c("CO2" = "co2", "Humedad" = "hum", "Material particulado" = "pm25", "Presión sonora" = "snd", "Temperatura" = "temp")

flux_filters_format <- function(named_measurement_tag_field = c("_field"="temp")) {
  glue::glue('r["{names(named_measurement_tag_field)}"] == "{named_measurement_tag_field}"') |> stringr::str_flatten(collapse = " or ")
}


## TODO:
##   - Desde el query que solo me retorne las columnas que necesito
##   - Revisar c´omo filtrar por variable no general r["_value"] < 100
query_builder_fn <- function(since_days = 1, fields_names = "temp", tags_names = c("location"="experimento"), measurement_names = c("_measurement"="tele"), value_max = "") {
  ifelse(value_max == "", filter_value_max <- "", filter_value_max <- glue::glue("|> filter(fn: (r) => r[\"_value\"] < {value_max})"))
  glue::glue('from(bucket: "senpork") |> range(start: -{since_days}d) |> filter(fn: (r) => {flux_filters_format(measurement_names)}) |> filter(fn: (r) => {flux_filters_format(tags_names)}) |> filter(fn: (r) => {flux_filters_format(fields_names)}) {filter_value_max} |> aggregateWindow(every: 1h, fn: mean, createEmpty: false) |> yield(name: "mean")')
}

fields <- c("co2", "hum", "pm25", "snd", "temp") |> purrr::set_names("_field")
locations <- c("experimento") |> purrr::set_names("location")
sensors <- c("sensor1","sensor2") |> purrr::set_names("sensor_id")


## Tener cuidado con la lista que devuelve, cada field por cada location
tables <- client$query(query_builder_fn(79, fields_names = fields, tags_names = locations, value_max = "")) |>
  purrr::map(tibble)

tables <- tables |>
  purrr::set_names(fields)

dygraph(tables$temp[,c("time","_value")])

tables[[9]]$sensor_id

## TODO: deal with nanotime type
temperatures <- tsibble(
  measurement = tables$temp$`_value`,
  time = ymd_hms(substr(tables$temp$`time`,1,19))
)

## https://github.com/tidyverse/ggplot2/issues/4288
g <- temperatures %>% autoplot()
ggplotly(g)


remove_duplicates <- duplicated(c("time"=substr(tables$temp$`time`,1,19)))
no_duplicates <- substr(tables$temp$`time`,1,19)
length(no_duplicates)

gg_season(temperatures |> fill_gaps())

ts1 <- ts1[-which(ts1$`_value` == ts1$`_value` |> max()), ]
