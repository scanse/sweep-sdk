module Main where

import Lib
import Control.Monad
import System.Environment
import Foreign.C.String

main :: IO ()
main = do
  args <- getArgs
  guard $ not $ null args
  let dev = head args

  version <- getVersion
  print version

  compatible <- isAbiCompatible
  print compatible

  let errorPtr = noErrorPtr

  device <- withCString dev $ flip deviceConstructSimple errorPtr

  checkError errorPtr >>= print

  startScanning device errorPtr
  checkError errorPtr >>= print

  scan <- getScan device errorPtr
  checkError errorPtr >>= print

  samples <- getSamples scan
  print samples

  void $ scanDestruct scan

  stopScanning device errorPtr
  checkError errorPtr >>= print

  void $ deviceDestruct device

  return ()
