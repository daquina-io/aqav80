if (!require(influxdbclient)) install.packages("influxdbclient")
if (!require(fpp3)) install.packages("fpp3")
if (!require(dygraphs)) install.packages("dygraphs")
if (!require(rmarkdown)) install.packages("rmarkdown")
library(purrr)
library(glue)
library(stringr)



options(browser = "/usr/bin/librewolf")

## You can generate an API token from the "API Tokens Tab" in the UI
## In your R console you can set token as
source("env.R")
token <- Sys.getenv("INFLUX_TOKEN")

client <- InfluxDBClient$new(
  url = "http://iot.unloquer.org:8086",
  token = token,
  org = "unloquer"
)

variables_names <- c("CO2" = "co2", "Humedad" = "hum", "Material particulado" = "pm25", "Presión sonora" = "snd", "Temperatura" = "temp")

flux_filters_format <- function(named_measurement_tag_field = c("_field"="temp"), aggregation_logic = "or") {
  glue::glue('r["{names(named_measurement_tag_field)}"] == "{named_measurement_tag_field}"') |> stringr::str_flatten(collapse = glue::glue(" {aggregation_logic} "))
}

## TODO:
##   - Desde el query que solo me retorne las columnas que necesito
##   - Revisar cómo filtrar por variable no general r["_value"] < 100
query_builder_fn <- function(since_days = 1, fields_names = c("_field"="temp"), tags_names = c("location"="experimento"), tags_aggregation_logic = "or", measurement_names = c("_measurement"="tele"), aggregate_window = "1h", value_max = "") {
  ifelse(value_max == "", filter_value_max <- "", filter_value_max <- glue::glue("|> filter(fn: (r) => r[\"_value\"] < {value_max})"))
  glue::glue('from(bucket: "senpork") |> range(start: -{since_days}d) |> filter(fn: (r) => {flux_filters_format(measurement_names)}) |> filter(fn: (r) => {flux_filters_format(tags_names, tags_aggregation_logic)}) |> filter(fn: (r) => {flux_filters_format(fields_names)}) {filter_value_max} |> aggregateWindow(every: {aggregate_window}, fn: mean, createEmpty: false) |> yield(name: "mean")')
}

fields <- c("co2", "hum", "pm25", "snd", "temp") |> purrr::set_names("_field")
locations <- c("experimento") |> purrr::set_names("location")
sensors <- c("sensor2") |> purrr::set_names("sensor_id")

## Tener cuidado con la lista que devuelve, cada field por cada location
tables <- client$query(query_builder_fn(
                   since_days = 125,
                   fields_names = fields,
                   tags_names = c(locations, sensors),
                   tags_aggregation_logic = "and",
                   aggregate_window = "30m",
                   value_max = 3000)) |>
  purrr::map(tibble)

tables <- tables |>
  purrr::set_names(fields)

dygraph(tables$temp[, c("time", "_value")], main = "Temperatura", group = "senpork") |> dySeries("_value", label ="Temperatura (C)") |> dyRangeSelector()
dygraph(tables$hum[, c("time", "_value")], main = "Humedad", group = "senpork") |> dySeries("_value", label ="Humedad (%)") |> dyRangeSelector()
dygraph(tables$co2[, c("time", "_value")],main = "CO2", group = "senpork") |> dySeries("_value", label ="CO2 (ppm)") |> dyRangeSelector()
dygraph(tables$pm25[, c("time", "_value")],main = "Material particulado", group = "lung-deaths") |> dySeries("_value", label ="Material particulado (µg/m3)") |> dyRangeSelector()
dygraph(tables$snd[, c("time", "_value")],main = "Presión sonora", group = "lung-deaths")


rmarkdown::render("./SenPork_analysis/report.Rmd")

## BOOK https://otexts.com/fpp3/
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
