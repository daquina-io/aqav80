if (!require(influxdbclient)) install.packages("influxdbclient")
if (!require(fpp3)) install.packages("fpp3")
library(influxdbclient)
library(fpp3)

## You can generate an API token from the "API Tokens Tab" in the UI
## In your R console you can set token as
## Sys.setenv(INFLUX_TOKEN="__generated_api_token__")
token <- Sys.getenv("INFLUX_TOKEN")

client <- InfluxDBClient$new(
  url = "http://iot.unloquer.org:8086",
  token = token,
  org = "unloquer"
)

fields <- c("CO2" = "co2", "Humedad" = "hum", "Material particulado" = "pm25", "Presión sonora" = "snd", "Temperatura" = "temp")

flux_field_format <- function(field = "temp") {
  glue::glue('r["_field"] == "{field}"')
}

flux_multiple_fields <- function(fields = "temp") {
  purrr::map(fields, flux_field_format) |> stringr::str_flatten(collapse = " or ")
}

## TODO:
##   - Desde el query que solo me retorne las columnas que necesito

query_builder_fn <- function(since_days = 1, fields_name = "temp", location_name = "experimento") {
  glue::glue('from(bucket: "senpork") |> range(start: -{since_days}d) |> filter(fn: (r) => r["_measurement"] == "tele") |> filter(fn: (r) => r["location"] == "{location_name}") |> filter(fn: (r) => {flux_multiple_fields(fields_name)}) |> yield(name: "mean")')
}

tables <- client$query(query_builder_fn(20, fields))

tables <- tables |>
  purrr::map(tibble) |>
  purrr::set_names(fields)

str(tables)


tables <- purrr::map(tables, tibble) |> purrr::set_names(fields)

tables$snd


str(tables)

str(tables[[1]] |> tibble())

df1 <- tables[[1]][c("time", "_value")]

ts1 <- tsibble(df1)

ts1 <- ts1[-which(ts1$`_value` == ts1$`_value` |> max()), ]

## https://github.com/tidyverse/ggplot2/issues/4288
ts1 %>% autoplot()

gg_season(ts1 |> fill_gaps())

capabilities()
dev.cur()

plot(1:100, runif(100))
