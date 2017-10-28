{-# LANGUAGE ForeignFunctionInterface #-}
{-# LANGUAGE DeriveAnyClass #-}

module Lib
    ( getVersion
    , isAbiCompatible
    , Error
    , noErrorPtr
    , checkError
    , errorDestruct
    , Device
    , deviceConstructSimple
    , deviceDestruct
    , startScanning
    , stopScanning
    , getScan
    , scanDestruct
    , getSamples
    ) where

import Foreign.C
import Foreign.Ptr
import Foreign.Storable


-- ABI

foreign import ccall unsafe "sweep_get_version"
    getVersion :: IO CInt

foreign import ccall unsafe "sweep_is_abi_compatible"
    isAbiCompatible :: IO CInt

-- Errors
--

newtype Error = Error (Ptr Error)
  deriving (Storable)

foreign import ccall unsafe "sweep_error_message"
    errorMessage :: Error -> IO CString

foreign import ccall unsafe "sweep_error_destruct"
    errorDestruct :: Error -> IO ()

noErrorPtr :: Ptr Error
noErrorPtr = nullPtr

checkError :: Ptr Error -> IO (Maybe String)
checkError errorPtr = do
  if errorPtr == noErrorPtr then
    pure $ Nothing
  else
    fmap Just (peek errorPtr >>= errorMessage >>= peekCString)


-- Device

newtype Device = Device (Ptr Device)

foreign import ccall unsafe "sweep_device_construct_simple"
    deviceConstructSimple :: CString -> Ptr Error -> IO Device

foreign import ccall unsafe "sweep_device_destruct"
    deviceDestruct :: Device -> IO ()

foreign import ccall unsafe "sweep_device_start_scanning"
    startScanning :: Device -> Ptr Error -> IO ()

foreign import ccall unsafe "sweep_device_stop_scanning"
    stopScanning :: Device -> Ptr Error -> IO ()

foreign import ccall unsafe "sweep_device_get_scan"
    getScan :: Device -> Ptr Error -> IO Scan

-- Scan

newtype Scan = Scan (Ptr Scan)

data Sample = Sample
  { sampleAngle    :: Int
  , sampleDistance :: Int }
  deriving(Show, Eq)

foreign import ccall unsafe "sweep_scan_destruct"
    scanDestruct :: Scan -> IO ()

foreign import ccall unsafe "sweep_scan_get_number_of_samples"
    getNumberOfSamples :: Scan -> IO CInt

foreign import ccall unsafe "sweep_scan_get_angle"
    getAngle :: Scan -> CInt -> IO CInt

foreign import ccall unsafe "sweep_scan_get_distance"
    getDistance :: Scan -> CInt -> IO CInt

getSamples :: Scan -> IO [Sample]
getSamples scan = do
  n <- getNumberOfSamples scan

  let samples = [0..n-1]

  sequence $ flip fmap samples (\ v -> do
    let convert = fromInteger . toInteger

    angle    <- getAngle    scan v
    distance <- getDistance scan v

    pure $ Sample { sampleAngle    = convert angle
                  , sampleDistance = convert distance })

